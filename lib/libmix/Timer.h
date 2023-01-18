
#ifndef _TIMER_H_
#define _TIMER_H_

#include <list>
#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>
#include <memory>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <algorithm>


class Timer
{
	typedef unsigned long long __ullong;
public:
	static const int ONE_SEC	= 1000;				//1s
	static const int ONE_MIN	= ONE_SEC * 60;		//1min
	static const int ONE_HOUR	= ONE_MIN * 60;		//1h
public:
	struct TimerNode
	{
		__ullong		_expect_time;		//超时时间 ms
		std::function<void(void)>  _call_fun;		//回调函数
	};
public:
	Timer(bool th = true)
		: m_need_thread(th)
		, m_start(false)
		, m_update_time(false)
	{
		m_wait_time = get_current_ms() + ONE_SEC * 100;
	}

	~Timer(void)
	{
		if (m_start){
			stopTimer();
		}
	}

public:
	static uint64_t get_current_ms();
	//启动定时器
	bool startTimer();
	//停止定时器
	void stopTimer();
	//添加一个定时器，返回一个定时器id用于后续操作
	long addTimer(const struct TimerNode& tmr);
	long addTimer(int expect,const std::function<void(void)>& fun);
	//修改一个定时器，通过id判断
	bool modTimer(long id,const struct TimerNode& tmr);
	//删除一个id对应的定时器
	bool delTimer(long id);

private:
	//运行定时器
	bool runTimer();
private:
	struct _timer_list
	{
		long				_id;
		struct TimerNode	_tmr;
	};

	bool						m_start;				//当前定时器是否启动
	bool						m_need_thread;			//是否是需要另起一个线程执行定时器
	bool						m_update_time;			//是否需要更新当前的睡眠时间
	__ullong					m_wait_time;			//当前需要到什么时间点才有任务执行
	std::list<std::shared_ptr<_timer_list>>		m_timer_list;
	std::mutex					m_timer_mutex;
	std::condition_variable		m_timer_cond;
	std::shared_ptr<std::thread>	m_pthr;


private:
	Timer(const Timer& t);
	void operator=(const Timer& t);
};

inline uint64_t Timer::get_current_ms()
{
	using namespace std::chrono;

	int64_t	cur_ms = 0;
	cur_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	return cur_ms;
}

inline bool Timer::startTimer()
{
	if (m_start){
		return true;
	}
	m_start = true;
	if (m_need_thread){
		m_pthr.reset(new std::thread(&Timer::runTimer, this));
		if (!m_pthr){
			return false;
		}
	}else{
		runTimer();
	}

	return true;
}

inline void Timer::stopTimer()
{
	if (m_start == false){
		return;
	}
	{
		std::lock_guard<std::mutex>	lk(m_timer_mutex);
		//		printf("当前定时器还有没有执行任务个数为 %d\n",m_timer_list.size());
		m_timer_list.clear();
		m_start = false;
	}

	m_timer_cond.notify_all();
	if (m_need_thread && m_pthr && m_pthr->joinable()){
		//		printf("等待定时器结束 %d\n",std::this_thread::get_id());
		m_pthr->join();
		//		printf("成功结束定时器线程\n");
	}
}

inline long Timer::addTimer(const struct TimerNode& tmr)
{
	if (m_start == false){
		return -1;
	}
	std::shared_ptr<_timer_list> ptmr(new _timer_list);
	if (ptmr == nullptr){
		return -1;
	}
	ptmr->_id = (long)(ptmr.get());
	ptmr->_tmr._call_fun = tmr._call_fun;
	ptmr->_tmr._expect_time = get_current_ms() + tmr._expect_time;

	//	printf("当前加入定时器将在 [ %llu ] MS超时\n",ptmr->_tmr._expect_time);

	std::lock_guard<std::mutex> lk(m_timer_mutex);
	m_timer_list.push_back(ptmr);
	m_timer_list.sort([](std::shared_ptr<_timer_list> _l, std::shared_ptr<_timer_list> _r){
		return _l->_tmr._expect_time < _r->_tmr._expect_time;
	});

	//判断当前的最新超时时间，是否小于现在正在睡眠的超时时间，如果小就唤醒
	if (ptmr->_tmr._expect_time < m_wait_time)
	{
		m_wait_time = ptmr->_tmr._expect_time;
		m_update_time = true;
		m_timer_cond.notify_all();
	}
	return ptmr->_id;
}

inline long Timer::addTimer(int expect, const std::function<void(void)>& fun)
{
	struct TimerNode tmr;
	tmr._expect_time = expect;
	tmr._call_fun = fun;
	return addTimer(tmr);
}

inline bool Timer::modTimer(long id, const struct TimerNode& tmr)
{
	if (m_start == false){
		return false;
	}

	std::lock_guard<std::mutex>	lk(m_timer_mutex);
	for (auto it = m_timer_list.begin(); it != m_timer_list.end(); ++it)
	{
		if ((*it)->_id == id)
		{
			(*it)->_tmr._call_fun = tmr._call_fun;
			(*it)->_tmr._expect_time = get_current_ms() + tmr._expect_time;
			m_timer_list.sort([](std::shared_ptr<_timer_list> _l, std::shared_ptr<_timer_list> _r){
				return _l->_tmr._expect_time < _r->_tmr._expect_time;
			});
			if ((*it)->_tmr._expect_time < m_wait_time)
			{
				m_wait_time = (*it)->_tmr._expect_time;
				m_update_time = true;
				m_timer_cond.notify_all();
			}
			return true;
		}
	}
	return false;
}

inline bool Timer::delTimer(long id)
{
	if (m_start == false){
		return false;
	}

	std::lock_guard<std::mutex>	lk(m_timer_mutex);
	for (auto it = m_timer_list.begin(); it != m_timer_list.end(); ++it)
	{
		if ((*it)->_id == id)
		{
			m_timer_list.erase(it);
			return true;
		}
	}
	return false;
}

inline bool Timer::runTimer()
{
	while (m_start)
	{

		std::unique_lock<std::mutex>	lk(m_timer_mutex);

		__ullong	 cur_time = get_current_ms();
		std::list<std::shared_ptr<_timer_list>>	exe_list;

		printf("准备睡眠....预计睡眠时间为 [ %lld ]\n", m_wait_time - cur_time);

		m_timer_cond.wait_for(lk, std::chrono::milliseconds(m_wait_time - cur_time), [this](){
			return m_update_time == true || m_start == false;
		});

		//		printf("定时线程被唤醒....当前MS [ %llu ]\n",cur_time);

		if (m_start == false){
			//			printf("定时器被停止.....退出线程 [ %d ]\n",std::this_thread::get_id());
			return true;
		}
		//唤醒后，执行队列任务
		if (m_update_time && !m_timer_list.empty()){
			m_wait_time = m_timer_list.front()->_tmr._expect_time;
			printf("由于时间间隔缩小，唤醒线程，更新睡眠时间为 [ %lld ]\n", m_wait_time - cur_time);
			m_update_time = false;
		}

		//如果当前时间大于等于队列中第一个元素的时间则开始执行队列中的定时任务
		cur_time = get_current_ms();
		for (auto _t_it = m_timer_list.begin(); _t_it != m_timer_list.end();)
		{
			if (cur_time >= (*_t_it)->_tmr._expect_time){
				exe_list.push_back(*_t_it);
				_t_it = m_timer_list.erase(_t_it);
			}
			else{
				break;
			}
		}

		lk.unlock();

		//开始执行定时任务
		for (auto& _tsk : exe_list)
		{
			if (_tsk->_tmr._call_fun){
				_tsk->_tmr._call_fun();
			}
		}


		//定时任务已经执行完毕，重新计算下一次任务的时间点
		cur_time = get_current_ms();

		lk.lock();
		if (!m_timer_list.empty()){
			m_wait_time = m_timer_list.front()->_tmr._expect_time;
		}
		else{
			m_wait_time = cur_time + ONE_SEC * 100;
		}
	}
	return false;
}

#endif
