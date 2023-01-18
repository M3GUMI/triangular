#pragma once

#include "config.hpp"

class DnsReslove
{
public:
	DnsReslove(ahttp::io_service& io) : io_(io) {}

	// DNS resolve server address to RpcEndpoint, only choose the first one.
	// @param io_service is the io service used for resolving.
	// @param host can be an ip or host name, eg. "127.0.0.1" or "baidu.com". 
	// @param svc can be a port or service name, eg. "21" or "ftp".
	// @param endpoints is out param, stores resolved RpcEndpoint if succeed,
	// but may be empty.
	// @return true if resolve succeed.
	// @return false if resolve failed.
	ahttp::error_code ResolveAddress(const std::string& host, const std::string& svc,
		ahttp::ip::tcp::endpoint& endpoint)
	{
		ahttp::ip::tcp::resolver resolver(io_);
		ahttp::error_code ec;
		ahttp::ip::tcp::resolver::iterator it, end;
		it = resolver.resolve(ahttp::ip::tcp::resolver::query(host, svc), ec);
		if (it != end){
			endpoint = it->endpoint();
		}
		return ec;
	}

	// DNS resolve server address to RpcEndpoint, only choose the first one.
	// @param io_service is the io service used for resolving.
	// @param server address should be in format of "host:port".
	// @param endpoint is out param, stores resolved RpcEndpoint if succeed.
	// @return true if resolve succeed.
	// @return false if resolve failed or not address found.
	ahttp::error_code ResolveAddress(const std::string& address,
		ahttp::ip::tcp::endpoint& endpoint)
	{
		std::string::size_type pos = address.find(':');
		if (pos == std::string::npos){
			return make_error_code(ahttp::errc::invalid_argument);
		}
		std::string host = address.substr(0, pos);
		std::string svc = address.substr(pos + 1);
		return ResolveAddress(host, svc, endpoint);
	}
private:
	ahttp::io_service& io_;
};

