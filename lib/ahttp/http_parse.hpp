#pragma once

#include "http_respone.hpp"

#include <string.h>
#include <memory>
#include <algorithm>

enum ParseResult
{
	result_not_enough = 0, // 数据长度不够
	result_get_success, // 成功解析出一条数据
	result_error,  // 数据格式错误
};

class HttpParse
{
public:
	HttpParse(){
		reset();
	}

	void reset()
	{
		clrf_count = 0;
		pre_char = 0;	
		status = parse_frist_line;
		first_header_line.clear();
		header_line.clear();
		context_length = 0;	
		body.clear();

		last_chunked = false; 
		ckstatus = chunked_head;
		ck_len_str.clear();
		ck_length = 0;

		result = std::make_shared<HttpRespone>();
	}

	HttpResponePtr get_respone() {
		return result;
	}

	ParseResult parse(const std::string& msg) 
	{
		const char* begin = msg.data();
		const char* end = begin + msg.size();
		ParseResult ret;
		switch (status)
		{
		case HttpParse::parse_frist_line:
			ret = process_first_header_line(begin, end);
			if (ret != result_get_success){
				return ret;
			}
		case HttpParse::parse_header_line:
			ret = process_header_line(begin, end);
			if (ret != result_get_success) {
				return ret;
			}
		default:
			return process_body(begin, end);
		}
		return result_error;
	}
private:
	ParseResult process_first_header_line(const char*& begin, const char* end)
	{
		for (; begin != end; begin++)
		{
			if (*begin == '\r'){
				;
			}else if (*begin == '\n'){
				if (pre_char != '\r'){
					return result_error;
				}
				else {
					clrf_count++;
					// 得到了正确的一行数据
					const char* str = first_header_line.c_str();
					auto pos = strstr(str, " ");
					if (pos == nullptr || pos + 1 == str + first_header_line.size()){
						return result_error;
					}
					result->set_http_version(std::string(str, pos));
					auto pos1 = strstr(pos + 1, " ");
					if (pos1 == nullptr) {
						return result_error;
					}
					try {
						result->set_http_status(std::stoi(std::string(pos + 1, pos1)));
					}catch (...) {}
					result->set_http_status_msg(pos1 + 1);
					status = parse_header_line;
					begin++;
					pre_char = 0;
					return result_get_success;
				}
			}
			else {
				first_header_line.append(begin, 1);
			}
			pre_char = *begin;
		}
		return result_not_enough;
	}

	ParseResult process_header_line(const char*& begin, const char* end)
	{
		for (; begin != end; begin++)
		{
			if (*begin == '\r') {
				;
			}
			else if (*begin == '\n') {
				if (pre_char != '\r') {
					return result_error;
				}
				else {
					clrf_count++;
					if (clrf_count == 2){
						status = parse_body;
						begin++;
						pre_char = 0;
						std::string ck;
						result->get_header("transfer-encoding", ck);
						if (ck == "chunked"){
							ckstatus = chunked_head;
							status = parse_chunked;
						}
						result->get_header("content-length", ck);
						if (!ck.empty()) {
							try {
								context_length = std::stoll(ck);
							}
							catch (const std::exception&) {
								return result_error;
							}
						}
						return result_get_success;
					}
					
					// 得到了数据，解析key-val
					if (!header_line.empty())
					{
						const char* str = header_line.c_str();
						auto pos = strstr(str, ":");
						if (pos == nullptr){
							return result_error;
						}
						auto tpos = pos;
						// 去掉key右边的空行
						for (; tpos != str && isspace(*tpos); tpos--);
						// 去掉val左边的空行
						for (pos++; pos != str + header_line.size() && isspace(*pos); pos++);
						std::string key(str, tpos);
						std::string val(pos);
						result->set_header(key, val);
					}
					header_line.clear();
				}
			}
			else {
				clrf_count = 0;
				header_line.append(begin, 1);
			}
			pre_char = *begin;
		}
		return result_not_enough;
	}

	ParseResult process_body(const char*& begin, const char* end)
	{
		if (status == parse_chunked){
			return process_chunked(begin, end);
		}
		return process_no_chunked(begin, end);
	}

	ParseResult process_no_chunked(const char*& begin, const char* end)
	{
		if (begin != end){
			body.append(begin, end - begin);
		}
		begin = end;
		if (body.size() == size_t(context_length)){
			result->set_payload(std::move(body));
			return result_get_success;
		}else if (body.size() > size_t(context_length)){
			return result_error;
		}
		return result_not_enough;
	}

	ParseResult process_chunked(const char*& begin, const char* end)
	{
		ParseResult ret;
		while (begin != end)
		{
			switch (ckstatus)
			{
			case chunked_head:
				ret = process_chunked_head(begin, end);
				if (ret != result_get_success) {
					return ret;
				}

			case chunked_body:
				ret = process_chunked_body(begin, end);
				if (ret != result_get_success) {
					return ret;
				}
			case chunked_foot:
				ret = process_chunked_foot(begin, end);
				if (ret == result_error) {
					return ret;
				}
				if (last_chunked && ret == result_get_success) {
					if (begin != end) {
						return result_error;
					}
					result->set_payload(std::move(body));
					return result_get_success;
				}
			default:
				break;
			}
		}
	
		return result_not_enough;
	}

	ParseResult process_chunked_head(const char*& begin, const char* end)
	{
		for (; begin != end; begin++)
		{
			switch (*begin)
			{
			case '\r':
				break;
			case '\n':
				if (pre_char == '\r')
				{
					++begin;
					pre_char = 0;
					ckstatus = chunked_body;
					try {
						ck_length = std::stoll(ck_len_str, 0, 16);
						ck_len_str.clear();
						last_chunked = ck_length == 0;
						return result_get_success;
					}
					catch (...) {
						return result_error;
					}
				}
				break;
			default:
				ck_len_str.append(begin, 1);
				break;
			}
			pre_char = *begin;
		}
		return result_not_enough;
	}

	ParseResult process_chunked_body(const char*& begin, const char* end)
	{
		if (last_chunked) {
			ckstatus = chunked_foot;
			pre_char = 0;
			return result_get_success;
		}
		int64_t cl = std::min<int64_t>(end - begin, ck_length);
		body.append(begin, size_t(cl));
		ck_length -= cl;
		begin += cl;
		if (ck_length == 0)
		{
			ckstatus = chunked_foot;
			pre_char = 0;
			return result_get_success;
		}
		return result_not_enough;
	}

	ParseResult process_chunked_foot(const char*& begin, const char* end)
	{
		for (; begin != end; begin++)
		{
			if (*begin == '\r') {
				;
			}
			else if (*begin == '\n') {
				if (pre_char != '\r') {
					return result_error;
				}
				begin++;
				pre_char = 0;
				ckstatus = chunked_head;
				return result_get_success;
			}
			pre_char = *begin;
		}
		return result_not_enough;
	}
private:
	enum ParseStatus {
		parse_frist_line = 0,
		parse_header_line,
		parse_body,
		parse_chunked,
	};
	int clrf_count = 0;
	char pre_char = 0;	// 上一次获取当的字符
	ParseStatus  status = parse_frist_line;
	std::string  first_header_line;
	std::string  header_line;
	int64_t		 context_length = 0;	// body 的长度
	std::string  body;

	// chunked 使用到的解析变量
	enum ChunkedStatus
	{
		chunked_head,
		chunked_body,
		chunked_foot,
	};
	bool last_chunked = false; // 最后一个数据块
	ChunkedStatus ckstatus = chunked_head;
	std::string ck_len_str;
	int64_t ck_length = 0;

	HttpResponePtr result;
};