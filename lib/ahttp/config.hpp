#pragma once

#ifdef ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <chrono>
namespace ahttp {
	using namespace asio;
	using std::errc;
	using std::error_code;
	using std::error_category;
	using std::error_condition;
	using std::system_error;
	using namespace std::chrono;
	using std::bind;
	namespace placeholders = std::placeholders;
}
#else
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/deadline_timer.hpp>
namespace ahttp {
	using namespace boost::asio;
	namespace errc = boost::system::errc;
	using boost::system::error_code;
	using boost::system::error_category;
	using boost::system::error_condition;
	using boost::system::system_error;
	using namespace boost::posix_time;
	using boost::bind;
	namespace placeholders {
		/// \todo this feels hacky, is there a better way?
		using ::_1;
		using ::_2;
		using ::_3;
		using ::_4;
		using ::_5;
	}
	typedef boost::asio::deadline_timer steady_timer;
}
#endif