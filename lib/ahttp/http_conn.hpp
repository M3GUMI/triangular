
#pragma once

#include "tcp_conn.hpp"
#include "http_parse.hpp"
#include "http_request.hpp"

#include <sstream>
#include <iostream>

class HttpConn;
typedef std::shared_ptr<HttpConn> HttpConnPtr;


typedef std::function<void(HttpResponePtr, const ahttp::error_code&)> HttpDoneCallback;


class HttpConn : public std::enable_shared_from_this<HttpConn>
{
public:
	HttpConn(ahttp::io_service& io) : io_(io), timer_(io) {}

	void set_timeout(int ms) {
		timeout_ = ms;
	}

	void set_proxy(const std::string& p) {
		proxy = std::make_shared<URL>(p);
	}

	void set_proxy(const std::shared_ptr<URL>& p) {
		proxy = p;
	}

	void set_local_endpoint(std::shared_ptr<ahttp::ip::tcp::endpoint> ep) {
		local_ep = ep;
	}

	bool available() const {
		return req_ == nullptr;
	}

	void Do(HttpRequestPtr req, HttpDoneCallback cb)
	{
		assert(req_ == nullptr);
		req_ = req;
		cb_ = cb;
		if (tcp_conn == nullptr)
		{
			if (req->Url.protocol() == "https") {
				tcp_conn = std::make_shared<SSLTcpConn>(io_);			
			}
			else if (req->Url.protocol() == "http") {
				tcp_conn = std::make_shared<PlainTcpConn>(io_);
			}
			if (proxy != nullptr){
				tcp_conn->set_proxy(proxy->host(), std::to_string(proxy->port()));
			}
			

			tcp_conn->set_timeout(timeout_);
			auto slf = shared_from_this();
			tcp_conn->set_connect_callback([this, slf](TcpConnPtr) {
				trigger_do_request();
			});
			tcp_conn->set_close_callback([this, slf](TcpConnPtr, const ahttp::error_code& ec) {
				printf("TCP DISCONNECT: %s\n", ec.message().c_str());
				tcp_conn.reset();
				if (cb_){
					cb_(nullptr, ec);
				}
				parser.reset();
			});
			tcp_conn->set_message_callback(std::bind(&HttpConn::tcp_message_handle,
				shared_from_this(), std::placeholders::_1, std::placeholders::_2));
			auto ec = tcp_conn->async_connect(req->Url.host(), req_->Url.port(), local_ep);
			if (ec) {
				cb(nullptr, ec);
				tcp_conn.reset();
				parser.reset();
			}
			return;
		}
		trigger_do_request();
	}

private:
	void reset_conn() 
	{
		if (tcp_conn != nullptr) {
			printf("Actively disconnect TCP connection\n");
			// tcp_conn 生命周期并不会马上结束
			// 如果不取消回调函数，在链接关闭后！可能还会回调到函数造成程序运行出现异常
			tcp_conn->set_connect_callback(nullptr);
			tcp_conn->set_close_callback(nullptr);
			tcp_conn->set_message_callback(nullptr);
			tcp_conn->close();
			tcp_conn.reset();
		}
	}

	void trigger_do_request()
	{
		assert(tcp_conn->status() == STATUS_CONNECTED && req_ != nullptr);
		if (timeout_ > 0) {
			timer_.expires_from_now(ahttp::milliseconds(timeout_));
			timer_.async_wait(ahttp::bind(&HttpConn::on_connect_timeout, shared_from_this(), ahttp::placeholders::_1));
		}
		std::stringstream sstr;
		sstr << req_->Method << " " << req_->Url.path();
		if (!req_->Url.query().empty()){
			sstr << "?" << req_->Url.query();
		}
		sstr << " HTTP/1.1\r\n";
		req_->Header["Host"] = req_->Url.host();
		req_->Header["Connection"] = "Keep-Alive";
		req_->Header["Content-Length"] = std::to_string(req_->Body.size());
		for (auto& h : req_->Header){
			sstr << h.first << ": " << h.second << "\r\n";
		}
		sstr << "\r\n";
		sstr << req_->Body;
		tcp_conn->send(sstr.str());
	}

	void tcp_message_handle(TcpConnPtr conn, const std::string& data) 
	{
		//std::cout << data;
		auto ret = parser.parse(data);
		ahttp::error_code ec;
		switch (ret)
		{
		case result_not_enough:
			break;
		case result_get_success:
			if (timeout_ > 0) {
				timer_.cancel(ec);
			}
			if (cb_){
				cb_(parser.get_respone(), ahttp::error_code());
			}
			
			parser.reset();
			req_.reset();
			cb_ = nullptr;
			break;
		case result_error:
			reset_conn();
			
			if (cb_) {
				cb_(nullptr, make_error_code(ahttp::errc::bad_message));
			}

			parser.reset();
			req_.reset();
			cb_ = nullptr;
			return;
		default:
			break;
		}
	}

	void on_connect_timeout(const ahttp::error_code& error)
	{
		if (error == ahttp::error::operation_aborted) {
			return;
		}
		printf("on_connect_timeout\n");
		reset_conn();

		if (cb_) {
			cb_(nullptr, make_error_code(ahttp::errc::timed_out));
		}
		req_.reset();
		cb_ = nullptr;
		parser.reset();
	}

private:
	ahttp::io_service& io_;
	TcpConnPtr tcp_conn;
	HttpParse parser;
	HttpRequestPtr req_;
	HttpDoneCallback cb_;

	std::shared_ptr<ahttp::ip::tcp::endpoint> local_ep;
	std::shared_ptr<URL> proxy;

	int timeout_ = 0;
	ahttp::steady_timer		timer_;
};