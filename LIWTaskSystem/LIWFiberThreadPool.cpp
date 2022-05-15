#include "LIWFiberThreadPool.h"

LIW::LIWFiberThreadPool::LIWFiberThreadPool():
	m_isRunning(false),
	m_isInit(false)
{

}

LIW::LIWFiberThreadPool::~LIWFiberThreadPool()
{

}

void LIW::LIWFiberThreadPool::Init(int numWorkers, int numFibers)
{
	Init(numWorkers, numWorkers, numFibers, numFibers);
}

void LIW::LIWFiberThreadPool::Init(int minWorkers, int maxWorkers, int minFibers, int maxFibers)
{
	//TODO: Make this adaptive somehow
	m_isRunning = true;

	for (int i = 0; i < minFibers; ++i) {
		LIWFiberWorker* worker = new LIWFiberWorker(i);
		m_fibers.push(worker);
		m_fibersRegistered.emplace_back(worker);
	}

	for (int i = 0; i < minWorkers; ++i) {
		m_workers.emplace_back(ProcessTask, this);
	}
	
	m_isInit = true;
}

bool LIW::LIWFiberThreadPool::Submit(LIWFiberTask* task)
{
	m_tasks.push(task);
	return true;
}

//TODO: Check still good for the fiber version
void LIW::LIWFiberThreadPool::WaitAndStop()
{
	m_isRunning = false;
	m_tasks.block_till_empty();
	for (auto& fiber : m_fibersRegistered) {
		fiber->Stop();
	}
	using namespace std::chrono;
	std::this_thread::sleep_for(1ms);
	m_tasks.notify_stop();

	for (int i = 0; i < m_workers.size(); ++i) {
		m_workers[i].join();
	}
}

//TODO: Check still good for the fiber version
void LIW::LIWFiberThreadPool::Stop()
{
	m_isRunning = false;

	LIWFiberTask* task;
	while (!m_tasks.empty()) {
		task = nullptr;
		m_tasks.pop(task);
		delete task;
	}

	for (auto& fiber : m_fibersRegistered) {
		fiber->Stop();
	}
	using namespace std::chrono;
	std::this_thread::sleep_for(1ms);
	m_tasks.notify_stop();

	for (int i = 0; i < m_workers.size(); ++i) {
		m_workers[i].join();
	}
}

void LIW::LIWFiberThreadPool::ProcessTask(LIWFiberThreadPool* thisTP)
{
	LIWFiberMain* fiberMain = LIWFiberMain::InitThreadMainFiber();
	LIWFiberTask* task = nullptr;
	LIWFiberWorker* fiber = nullptr;
	while (!thisTP->m_tasks.empty() || thisTP->m_isRunning) {
		if (thisTP->m_fibers.pop(fiber) >= 0) {
			if (thisTP->m_tasks.pop(task) >= 0) {
				fiber->SetMainFiber(fiberMain);
				fiber->SetRunFunction(task->m_runner, task->m_param);
				fiberMain->YieldTo(fiber);
				delete task;
			}
			thisTP->m_fibers.push(fiber);
		}
	}
}
