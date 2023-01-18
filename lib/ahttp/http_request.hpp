#pragma once

#include "url.hpp"

#include <map>
#include <memory>

class HttpRequest;
typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

class HttpRequest
{
public:
	static HttpRequestPtr make_request(const std::string& method, const std::string& url, const std::string& body)
	{
		HttpRequestPtr req = std::make_shared<HttpRequest>();
		req->Method = method;
		req->Body = body;
		req->Url = URL::from_string(url);

		return req;
	}
public:
	std::map<std::string, std::string> Header;
	std::string Method = "GET";
	std::string Body;
	URL  Url;
};