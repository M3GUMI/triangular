
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <mutex>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
//#include "Closure.h"




class ThreadPool
{
	typedef std::function<void(void)> callback;
public:
	ThreadPool(int thread_num = 1);
	~ThreadPool(void);

public:

	bool start();
	void stop();
	bool addTask(const callback& cmd);
private:
	void run();
	callback takeTask();
private:
	int										m_thread_count;
	bool									m_is_exit;
	bool									m_is_start;

	std::mutex								m_mutex;
	std::condition_variable					m_cond;
	std::list<std::thread>					m_threads;
	std::list<callback>		m_tasks;
private:
	ThreadPool(ThreadPool&);
	ThreadPool& operator=(ThreadPool&);
};

inline ThreadPool::ThreadPool(int thread_num)
	: m_thread_count(thread_num)
	, m_is_start(false)
	, m_is_exit(false)
{
}


inline ThreadPool::~ThreadPool(void)
{
	if (m_is_start){
		stop();
	}
}

inline bool ThreadPool::start()
{
	//启动线程
	m_is_start = true;

	for (int i = 0; i < m_thread_count; ++i)
	{
		m_threads.push_back(std::thread(&ThreadPool::run, this));
	}
	return true;
}

inline void ThreadPool::stop()
{
	//唤醒所以线程结束线程
	m_is_exit = true;
	m_is_start = false;
	m_cond.notify_all();

	for (auto &thr : m_threads)
	{
		if (thr.joinable()){
			thr.join();
		}
	}
	m_threads.clear();
	m_tasks.clear();
}

inline bool ThreadPool::addTask(const callback& cmd)
{
	if (m_is_start == false){
		return false;
	}

	{
		std::lock_guard<std::mutex>	lk(m_mutex);
		m_tasks.push_back(cmd);
	}

	m_cond.notify_one();
	return true;
}

inline void ThreadPool::run()
{
	printf("[ %d ] 线程启动......\n", std::this_thread::get_id());
	while (m_is_exit == false)
	{
		callback tsk = takeTask();
		if (tsk){
			tsk();
		}
	}

	printf("[ %d ] 线程退出......\n", std::this_thread::get_id());
}

inline ThreadPool::callback ThreadPool::takeTask()
{
	callback clos;
	if (m_is_start == false){
		return clos;
	}


	std::unique_lock<std::mutex>	lk(m_mutex);
	//////如果队列不为空，或者程序退出则醒过来
	m_cond.wait(lk, [this](){
		return !m_tasks.empty() || m_is_exit == true;
	});

	////如果线程退出则直接返回
	if (m_is_exit){
		return clos;
	}

	////取出第一个元素返回
	clos = m_tasks.front();
	m_tasks.pop_front();
	return clos;
}

#endif