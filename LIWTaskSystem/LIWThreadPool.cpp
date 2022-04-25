#include "LIWThreadPool.h"

LIW::LIWThreadPool::LIWThreadPool():
	__m_isRunning(false),
	__m_isInit(false)
{
	
}

LIW::LIWThreadPool::~LIWThreadPool() {
	//TODO: Should probably do some loose thread handling?
}


void LIW::LIWThreadPool::Init(int numWorkers)
{
	Init(numWorkers, numWorkers);
}

void LIW::LIWThreadPool::Init(int minWorkers, int maxWorkers)
{
	__m_isRunning = true;
	for (int i = 0; i < minWorkers; ++i) {
		m_workers.emplace_back(std::thread(std::bind(&LIW::LIWThreadPool::ProcessTask, this)));
	}
	__m_isInit = true;
}

bool LIW::LIWThreadPool::Submit(LIWITask* task)
{
	m_tasks.push(task);
	return true;
}

void LIW::LIWThreadPool::WaitAndStop()
{
	__m_isRunning = false;
	m_tasks.block_till_empty();
	using namespace std::chrono;
	std::this_thread::sleep_for(1ms);
	m_tasks.notify_stop();

	for (int i = 0; i < m_workers.size(); ++i) {
		m_workers[i].join();
	}
}

void LIW::LIWThreadPool::Stop()
{
	__m_isRunning = false;

	LIWITask* task;
	while (!m_tasks.empty()) {
		task = nullptr;
		m_tasks.pop(task);
		delete task;
	}

	using namespace std::chrono;
	std::this_thread::sleep_for(1ms);
	m_tasks.notify_stop();

	for (int i = 0; i < m_workers.size(); ++i) {
		m_workers[i].join();
	}
}

#include <iostream>
#include <string>
#include <sstream>

void LIW::LIWThreadPool::ProcessTask()
{
	//TODO: Do task cleaning somewhere
	while (!m_tasks.empty() || __m_isRunning) {
		LIWITask* task = nullptr;
		if (m_tasks.pop(task) >= 0) {
			task->Execute(nullptr);
			delete task;
		}
	}
}
