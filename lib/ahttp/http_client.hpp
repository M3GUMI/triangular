#pragma once

#include "http_conn.hpp"
#include <map>
#include <vector>

class HttpClient
{
public:
	HttpClient(ahttp::io_service& io) :io_(io) {}

	void set_timeout(int ms) {
		time_out = ms;
	}

	void set_ippool(const std::vector<std::string>& ips) {
		local_ips.clear();
		next_ip = 0;
		for (auto& ip : ips)
		{
			ahttp::error_code ec;
			auto addr = ahttp::ip::address::from_string(ip, ec);
			if (!ec)
			{
				auto ep = std::make_shared<ahttp::ip::tcp::endpoint>(addr, 0);
				local_ips.push_back(ep);
			}
		}
	}

	void set_proxy(const std::string& p) {
		proxy = std::make_shared<URL>(p);
	}

	void Do(HttpRequestPtr req, HttpDoneCallback cb)
	{
		auto conn = find_conn(req);
		conn->Do(req, cb);
	}

private:
	HttpConnPtr find_conn(const HttpRequestPtr& req)
	{
		std::stringstream key;
		key << req->Url.host() << ":" << req->Url.port();
		for (auto& vconn : conn_pool)
		{
			for (auto& conn : vconn.second)
			{
				if (conn->available()) {
					conn->set_proxy(proxy);
					return conn;
				}
			}
		}
		
		HttpConnPtr conn = std::make_shared<HttpConn>(io_);
		if (!local_ips.empty()) {
			conn->set_local_endpoint(local_ips[next_ip++ % local_ips.size()]);
		}
		conn_pool[key.str()].push_back(conn);
		conn->set_proxy(proxy);
		conn->set_timeout(time_out);
		return conn;
	}
private:
	ahttp::io_service& io_;
	std::map<std::string, std::vector<HttpConnPtr>> conn_pool;
	int time_out = 0;

	std::vector<std::shared_ptr<ahttp::ip::tcp::endpoint>> local_ips;
	int64_t next_ip = 0;
	std::shared_ptr<URL> proxy;
};