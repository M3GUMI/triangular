#pragma once

#include <cctype>
#include <map>
#include <string>
#include <memory>


class HttpRespone;
typedef std::shared_ptr<HttpRespone> HttpResponePtr;


class HttpRespone
{
public:
	void set_payload(std::string&& p) {
		payload_ = p;
	}
	void set_payload(const std::string& p) {
		payload_ = p;
	}
	const std::string& payload() const {
		return payload_;
	}
	void set_http_version(const std::string& hv) {
		http_version_ = hv;
	}
	const std::string& http_version() const {
		return http_version_;
	}
	void set_http_status(int hs) {
		http_status_ = hs;
	}
	int http_status() const {
		return http_status_;
	}
	void set_http_status_msg(const std::string& hsm) {
		http_status_msg_ = hsm;
	}
	const std::string& http_status_msg() const {
		return http_status_msg_;
	}
	void set_header(const std::string& key, const std::string& val){
		header_map_[to_lower(key)] = val;
	}
	void get_header(const std::string& key, std::string& val) const 
	{
		val.clear();
		auto lkey = to_lower(key);
		auto it = header_map_.find(lkey);
		if (it != header_map_.end()){
			val = it->second;
		}
	}

public:
	static std::string to_lower(const std::string& str)
	{
		std::string ret(str);
		for (size_t i = 0; i < ret.size(); ++i) {
			if (isupper(ret[i])) {
				ret[i] = tolower(ret[i]);
			}
		}
		return ret;
	}
private:
	std::string payload_;
	std::string http_version_;
	int			http_status_;
	std::string http_status_msg_;
	std::map<std::string, std::string> header_map_;
};