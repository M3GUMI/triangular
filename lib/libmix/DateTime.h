#pragma once

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>
#include <chrono>


class DateTime
{
public:
	DateTime() { memset(&m_tm, 0, sizeof(m_tm)); }

	DateTime(time_t tim)
	{ 
		m_time = tim;
		memset(&m_tm, 0, sizeof(m_tm));
#ifdef _WIN32
		localtime_s(&m_tm, &tim);
#else
		localtime_r(&tim, &m_tm);
#endif
	}

	// 接受  如 2000-01-01 01:01:01.001 或者 Sep 18 2010 10:59:19 格式的字符串
	DateTime(const std::string& time_str)
	{ 
		memset(&m_tm, 0, sizeof(m_tm));

		if (isdigit(time_str[0])) 
		{
			int i = sscanf(time_str.c_str(), "%d-%d-%d%*c%d:%d:%d.%d",
				&(m_tm.tm_year), &(m_tm.tm_mon), &(m_tm.tm_mday),
				&(m_tm.tm_hour), &(m_tm.tm_min), &(m_tm.tm_sec), &m_milsec);
		}
		else 
		{
			char mon[4] = {};
			sscanf(time_str.c_str(), "%3s %d %d%*c%d:%d:%d.%d",
				mon, &(m_tm.tm_mday), &(m_tm.tm_year),
				&(m_tm.tm_hour), &(m_tm.tm_min), &(m_tm.tm_sec), &m_milsec);
			// "Jan","Feb","Mar","Apr","May","Jun","Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
			if (strncmp(mon, "Jan", 4) == 0) m_tm.tm_mon = 1;
			else if (strncmp(mon, "Feb", 4) == 0) m_tm.tm_mon = 2;
			else if (strncmp(mon, "Mar", 4) == 0) m_tm.tm_mon = 3;
			else if (strncmp(mon, "Apr", 4) == 0) m_tm.tm_mon = 4;
			else if (strncmp(mon, "May", 4) == 0) m_tm.tm_mon = 5;
			else if (strncmp(mon, "Jun", 4) == 0) m_tm.tm_mon = 6;
			else if (strncmp(mon, "Jul", 4) == 0) m_tm.tm_mon = 7;
			else if (strncmp(mon, "Aug", 4) == 0) m_tm.tm_mon = 8;
			else if (strncmp(mon, "Sep", 4) == 0) m_tm.tm_mon = 9;
			else if (strncmp(mon, "Oct", 4) == 0) m_tm.tm_mon = 10;
			else if (strncmp(mon, "Nov", 4) == 0) m_tm.tm_mon = 11;
			else if (strncmp(mon, "Dec", 4) == 0) m_tm.tm_mon = 12;
		}
		m_tm.tm_year -= 1900;
		m_tm.tm_mon--;
		m_tm.tm_isdst = -1;
		m_time = mktime(&m_tm);
	}

    static DateTime currentDateTime() {
        return DateTime(time(NULL));
    }

    static uint64_t currenctTimeMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

	static uint64_t currenctTimeUs() {
		using namespace std::chrono;
		return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
	}

	time_t Unix() const { return m_time; }

	int64_t Millisecond() const {
		return m_time * 1000 + m_milsec;
	}

	std::string toString() const
	{ 
		char buffer[128] = { 0 };
		strftime(buffer, 128, "%Y-%m-%d %H:%M:%S", &m_tm);
		return buffer;
	}

	int year() const { return m_tm.tm_year + 1900; }

	int month() const { return m_tm.tm_mon + 1; }

	int day() const { return m_tm.tm_mday; }

	int weeks() const { return m_tm.tm_wday; }

	int hour() const { return m_tm.tm_hour; }

	int min() const { return m_tm.tm_min; }

	int sec() const{ return m_tm.tm_sec; }


    DateTime operator+(const DateTime& other) const {
        return DateTime(this->m_time + other.m_time);
    }

    DateTime operator-(const DateTime& other) const {
        return DateTime(this->m_time - other.m_time);
    }

    DateTime& operator+=(const DateTime& other) {
        *this = *this + other;
        return *this;
    }

    DateTime& operator-=(const DateTime& other) {
        *this = *this - other;
        return *this;
    }

	bool operator<(const DateTime& other) const{ return m_time < other.m_time; }

	bool operator>(const DateTime& other) const { return m_time > other.m_time; }

	bool operator==(const DateTime& other) const { return m_time == other.m_time; }

	bool operator!=(const DateTime& other) const { return m_time != other.m_time; }

	bool operator<=(const DateTime& other) const { return m_time <= other.m_time; }

	bool operator>=(const DateTime& other) const { return m_time >= other.m_time; }

private:
	time_t		m_time = 0;
	int			m_milsec = 0;
	struct tm	m_tm;
};




