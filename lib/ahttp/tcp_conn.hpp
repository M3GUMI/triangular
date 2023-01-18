#pragma once

#include "config.hpp"
#include <memory>
#include <functional>
#include <system_error>

#include "dns_reslove.hpp"

class TcpConn;
typedef std::shared_ptr<TcpConn> TcpConnPtr;

typedef std::function<void(TcpConnPtr)> ConnectCallback;
typedef std::function<void(TcpConnPtr, const ahttp::error_code&)> CloseCallback;
typedef std::function<void(TcpConnPtr, const std::string&)> MessageCallback;

enum CONNECT_STATUS
{
	STATUS_INIT = 0,
	STATUS_CONNECTING = 1,
	STATUS_CONNECTED = 2,
	STATUS_CLOSED = 3,
};

class TcpConn : public std::enable_shared_from_this<TcpConn>
{
public:
	TcpConn(ahttp::io_service& io) : io_(io), dns_(io), timer_(io){}

	virtual ~TcpConn() {}

	CONNECT_STATUS status() const {
		return status_;
	}

	void set_connect_callback(const ConnectCallback& cb) {
		connect_cb = cb;
	}

	void set_close_callback(const CloseCallback& cb) {
		close_cb = cb;
	}

	void set_message_callback(const MessageCallback& cb) {
		message_cb = cb;
	}

	void set_timeout(int ms) {
		timeout_ = ms;
	}

	void set_proxy(const std::string& host, const std::string& port) {
		auto ec = dns_.ResolveAddress(host, port, proxy_point_);
		if (!ec){
			use_proxy_ = true;
		}
	}

	void close() 
	{
		proxy_svr_msg.clear();
		send_len = 0;
		send_buf.clear();
		ahttp::error_code ec;
		next_layer().close(ec);
		status_ = STATUS_CLOSED;
	}

	void send(const std::string& msg)
	{
		send_buf.append(msg);
		trigger_send();
	}

	ahttp::error_code async_connect(const std::string& host, uint16_t port, std::shared_ptr<ahttp::ip::tcp::endpoint> local_addr)
	{
		assert(status_ != STATUS_CONNECTING && status_ != STATUS_CONNECTED);
		auto ec = dns_.ResolveAddress(host, std::to_string(port), remote_point_);
		if (ec) {
			return ec;
		}
		remote_host_ = host;
		remote_port_ = port;
		auto slf = shared_from_this();
		status_ = STATUS_CONNECTING;
		trigger_async_connect(local_addr);
		if (timeout_ > 0) {
			timer_.expires_from_now(ahttp::milliseconds(timeout_));
			timer_.async_wait(std::bind(&TcpConn::on_connect_timeout, slf, std::placeholders::_1));
		}
		return ahttp::error_code();
	}

	
public:

	void on_connect(const ahttp::error_code& error)
	{
		//Maybe already timeout
		if (status_ != STATUS_CONNECTING) {
			return;
		}

		if (error) {
			close();
			if (close_cb)
				close_cb(shared_from_this(), error);
			return;
		}

		ahttp::error_code ec;
		next_layer().set_option(ahttp::ip::tcp::no_delay(true), ec);
		if (ec) {
			close();
			if (close_cb)
				close_cb(shared_from_this(), ec);
			return;
		}

		status_ = STATUS_CONNECTED;
		timer_.cancel();
		if (connect_cb) {
			connect_cb(shared_from_this());
		}

		trigger_recv();
		trigger_send();
	}

	void on_connect_timeout(const ahttp::error_code& error)
	{
		if (status_ != STATUS_CONNECTING) {
			return;
		}
		if (error == ahttp::error::operation_aborted) {
			return;
		}
		close();
		if (close_cb)
			close_cb(shared_from_this(), make_error_code(ahttp::errc::timed_out));
	}

	void on_recv(const ahttp::error_code& error, std::size_t bytes_transferred)
	{

		if (error) {
			close();
			if (error != ahttp::error::operation_aborted && close_cb) {
				close_cb(shared_from_this(), error);
			}
			return;
		}

		if (message_cb) {
			message_cb(shared_from_this(), std::string(recv_buf, bytes_transferred));
		}
		trigger_recv();
	}

	void on_send(const ahttp::error_code& error, std::size_t bytes_transferred)
	{
		sending = false;
		if (error) {
			close();
			if (error != ahttp::error::operation_aborted && close_cb) {
				close_cb(shared_from_this(), error);
			}
			return;
		}

		send_len += bytes_transferred;
		if (send_len == send_buf.size()) {
			send_buf.clear();
			send_len = 0;
			return;
		}

		trigger_send();
	}

	void proxy_on_connect(std::function<void(const ahttp::error_code&)> cb, const ahttp::error_code& error)
	{
		if (error){
			cb(error);
			return;
		}
		// 发送http代理握手请求
		std::stringstream istr;
		
		istr << "CONNECT " << remote_host_ << ":" << remote_port_ << " HTTP/1.1\r\n";
		istr << "Host: " << remote_point_.address() << ":" << remote_point_.port() << "\r\n";
		istr << "Proxy-Connection: Keep-Alive\r\n";
		istr << "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36\r\n";
		istr << "Proxy-authorization: Basic *\r\n";
		istr <<"\r\n";
		ahttp::error_code ec;
		ahttp::write(next_layer(), ahttp::buffer(istr.str()), ec);
		ahttp::streambuf response;
		ahttp::read_until(next_layer(), response, "\r\n\r\n", ec);
		// 得到http响应码
		std::string _;
		int code = 0;
		std::istream ostr(&response);
		ostr >> _ >> code;
		if (code != 200) {
			cb(make_error_code(ahttp::errc::bad_message));
			return;
		}
		
		cb(ahttp::error_code());
	}

private:
	virtual ahttp::ip::tcp::socket& next_layer() = 0;

	virtual void trigger_async_connect(std::shared_ptr<ahttp::ip::tcp::endpoint> local_addr) = 0;

	virtual void trigger_recv() = 0;

	virtual void trigger_send() = 0;

protected:

	CONNECT_STATUS		status_ = STATUS_INIT;

	ahttp::io_service& io_;

	DnsReslove dns_;

	int timeout_ = 0;
	ahttp::steady_timer		timer_;

	bool							use_proxy_ = false;
	ahttp::ip::tcp::endpoint  proxy_point_;
	std::string  proxy_svr_msg;

	std::string remote_host_;
	uint16_t	remote_port_ = 0;
	ahttp::ip::tcp::endpoint  remote_point_;

	ConnectCallback connect_cb;
	CloseCallback close_cb;
	MessageCallback message_cb;

	char	recv_buf[4096];

	bool	sending = false;
	size_t	send_len = 0;
	std::string send_buf;
};



class PlainTcpConn : public TcpConn
{
public:
	PlainTcpConn(ahttp::io_service& io) : TcpConn(io)
	{
		sock_ = std::make_shared<ahttp::ip::tcp::socket>(io);
	}
	
private:
	virtual ahttp::ip::tcp::socket& next_layer() {
		return *sock_;
	}

	virtual void trigger_async_connect(std::shared_ptr<ahttp::ip::tcp::endpoint> local_addr)
	{
		if (local_addr != nullptr) {
			sock_ = std::make_shared<ahttp::ip::tcp::socket>(io_, *local_addr);
			//printf("%s:%d\n", sock_->local_endpoint().address().to_string().c_str(), sock_->local_endpoint().port());
		}
		if (use_proxy_){
			std::function<void(const ahttp::error_code&)> cb = std::bind(&TcpConn::on_connect, shared_from_this(), std::placeholders::_1);
			sock_->async_connect(proxy_point_, ahttp::bind(&TcpConn::proxy_on_connect, shared_from_this(), cb, ahttp::placeholders::_1));
			return;
		}
		sock_->async_connect(remote_point_, ahttp::bind(&TcpConn::on_connect, shared_from_this(), ahttp::placeholders::_1));
	}

	virtual void trigger_recv()
	{
		if (status_ != STATUS_CONNECTED){
			return;
		}
		sock_->async_read_some(ahttp::buffer(recv_buf, sizeof(recv_buf)),
			ahttp::bind(&TcpConn::on_recv, shared_from_this(), ahttp::placeholders::_1, ahttp::placeholders::_2));
	}

	virtual void trigger_send()
	{
		if (!sending && status_ == STATUS_CONNECTED && !send_buf.empty()) {
			sending = true;
			sock_->async_write_some(ahttp::buffer(send_buf.data() + send_len, send_buf.size() - send_len),
				ahttp::bind(&TcpConn::on_send, shared_from_this(), ahttp::placeholders::_1, ahttp::placeholders::_2));
		}
	}
private:
	std::shared_ptr<ahttp::ip::tcp::socket> sock_;
};




class SSLTcpConn : public TcpConn
{
public:
	SSLTcpConn(ahttp::io_service& io) : TcpConn(io)
	{
		ssl_ctx = std::make_shared<ahttp::ssl::context>(ahttp::ssl::context::sslv23_client);
		ssl_ctx->set_verify_mode(ahttp::ssl::verify_none);
		sock_ = std::make_shared<ahttp::ssl::stream<ahttp::ip::tcp::socket>>(io, *ssl_ctx);
	}

	
private:
	virtual ahttp::ip::tcp::socket& next_layer() {
		return sock_->next_layer();
	}

	virtual void trigger_async_connect(std::shared_ptr<ahttp::ip::tcp::endpoint> local_addr)
	{
		if (local_addr != nullptr){
			sock_ = std::make_shared<ahttp::ssl::stream<ahttp::ip::tcp::socket>>(ahttp::ip::tcp::socket(io_, *local_addr), *ssl_ctx);
			//printf("%s:%d\n", sock_->lowest_layer().local_endpoint().address().to_string().c_str(), sock_->lowest_layer().local_endpoint().port());
		}
		SSL_set_tlsext_host_name(sock_->native_handle(), remote_host_.c_str());
		std::function<void(const ahttp::error_code&)> cb = std::bind(&SSLTcpConn::on_handle_shake, std::static_pointer_cast<SSLTcpConn>(shared_from_this()), std::placeholders::_1);
		if (use_proxy_){
			sock_->lowest_layer().async_connect(proxy_point_, ahttp::bind(&TcpConn::proxy_on_connect, shared_from_this(), cb, ahttp::placeholders::_1));
			return;
		}
		sock_->lowest_layer().async_connect(remote_point_, ahttp::bind(&SSLTcpConn::on_handle_shake,
			std::static_pointer_cast<SSLTcpConn>(shared_from_this()), ahttp::placeholders::_1));
	}

	virtual void trigger_recv()
	{
		if (status_ != STATUS_CONNECTED) {
			return;
		}
		sock_->async_read_some(ahttp::buffer(recv_buf, sizeof(recv_buf)),
			ahttp::bind(&TcpConn::on_recv, shared_from_this(), ahttp::placeholders::_1, ahttp::placeholders::_2));
	}

	virtual void trigger_send()
	{
		if (!sending && status_ == STATUS_CONNECTED && !send_buf.empty()) {
			sending = true;
			sock_->async_write_some(ahttp::buffer(send_buf.data() + send_len, send_buf.size() - send_len),
				ahttp::bind(&TcpConn::on_send, shared_from_this(), ahttp::placeholders::_1, ahttp::placeholders::_2));
		}
	}

	void on_handle_shake(const ahttp::error_code& error)
	{
		if (error){
			// error
			on_connect(error);
			return;
		}
		
		sock_->async_handshake(ahttp::ssl::stream_base::client,
			ahttp::bind(&TcpConn::on_connect, shared_from_this(), ahttp::placeholders::_1));
	}
private:
	std::shared_ptr<ahttp::ssl::context> ssl_ctx;
	std::shared_ptr<ahttp::ssl::stream<ahttp::ip::tcp::socket>> sock_;
};