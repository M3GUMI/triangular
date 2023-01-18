
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
		__ullong		_expect_time;		//��ʱʱ�� ms
		std::function<void(void)>  _call_fun;		//�ص�����
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
	//������ʱ��
	bool startTimer();
	//ֹͣ��ʱ��
	void stopTimer();
	//���һ����ʱ��������һ����ʱ��id���ں�������
	long addTimer(const struct TimerNode& tmr);
	long addTimer(int expect,const std::function<void(void)>& fun);
	//�޸�һ����ʱ����ͨ��id�ж�
	bool modTimer(long id,const struct TimerNode& tmr);
	//ɾ��һ��id��Ӧ�Ķ�ʱ��
	bool delTimer(long id);

private:
	//���ж�ʱ��
	bool runTimer();
private:
	struct _timer_list
	{
		long				_id;
		struct TimerNode	_tmr;
	};

	bool						m_start;				//��ǰ��ʱ���Ƿ�����
	bool						m_need_thread;			//�Ƿ�����Ҫ����һ���߳�ִ�ж�ʱ��
	bool						m_update_time;			//�Ƿ���Ҫ���µ�ǰ��˯��ʱ��
	__ullong					m_wait_time;			//��ǰ��Ҫ��ʲôʱ����������ִ��
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
		//		printf("��ǰ��ʱ������û��ִ���������Ϊ %d\n",m_timer_list.size());
		m_timer_list.clear();
		m_start = false;
	}

	m_timer_cond.notify_all();
	if (m_need_thread && m_pthr && m_pthr->joinable()){
		//		printf("�ȴ���ʱ������ %d\n",std::this_thread::get_id());
		m_pthr->join();
		//		printf("�ɹ�������ʱ���߳�\n");
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

	//	printf("��ǰ���붨ʱ������ [ %llu ] MS��ʱ\n",ptmr->_tmr._expect_time);

	std::lock_guard<std::mutex> lk(m_timer_mutex);
	m_timer_list.push_back(ptmr);
	m_timer_list.sort([](std::shared_ptr<_timer_list> _l, std::shared_ptr<_timer_list> _r){
		return _l->_tmr._expect_time < _r->_tmr._expect_time;
	});

	//�жϵ�ǰ�����³�ʱʱ�䣬�Ƿ�С����������˯�ߵĳ�ʱʱ�䣬���С�ͻ���
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

		printf("׼��˯��....Ԥ��˯��ʱ��Ϊ [ %lld ]\n", m_wait_time - cur_time);

		m_timer_cond.wait_for(lk, std::chrono::milliseconds(m_wait_time - cur_time), [this](){
			return m_update_time == true || m_start == false;
		});

		//		printf("��ʱ�̱߳�����....��ǰMS [ %llu ]\n",cur_time);

		if (m_start == false){
			//			printf("��ʱ����ֹͣ.....�˳��߳� [ %d ]\n",std::this_thread::get_id());
			return true;
		}
		//���Ѻ�ִ�ж�������
		if (m_update_time && !m_timer_list.empty()){
			m_wait_time = m_timer_list.front()->_tmr._expect_time;
			printf("����ʱ������С�������̣߳�����˯��ʱ��Ϊ [ %lld ]\n", m_wait_time - cur_time);
			m_update_time = false;
		}

		//�����ǰʱ����ڵ��ڶ����е�һ��Ԫ�ص�ʱ����ʼִ�ж����еĶ�ʱ����
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

		//��ʼִ�ж�ʱ����
		for (auto& _tsk : exe_list)
		{
			if (_tsk->_tmr._call_fun){
				_tsk->_tmr._call_fun();
			}
		}


		//��ʱ�����Ѿ�ִ����ϣ����¼�����һ�������ʱ���
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
