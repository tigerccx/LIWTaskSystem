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
		m_fibers.push_now(worker);
		m_fibersRegistered.emplace_back(worker);
	}

	for (int i = 0; i < minWorkers; ++i) {
		m_workers.emplace_back(ProcessTask, this);
	}
	
	m_isInit = true;
}

void LIW::LIWFiberThreadPool::WaitAndStop()
{
	m_isRunning = false;
	m_tasks.block_till_empty();
	m_fibersAwakeList.block_till_empty();
	for (auto& fiber : m_fibersRegistered) {
		fiber->Stop();
	}
	using namespace std::chrono;
	std::this_thread::sleep_for(1ms);
	m_tasks.notify_stop();
	m_fibersAwakeList.notify_stop();

	for (int i = 0; i < m_workers.size(); ++i) {
		m_workers[i].join();
	}
}

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

LIW::LIWFiberThreadPool::counter_type LIW::LIWFiberThreadPool::DecreaseSyncCounter(counter_size_type idxCounter, counter_type decrease)
{
	LIWFiberSyncCounter& counter = m_syncCounters[idxCounter];
	int val = counter.m_counter.fetch_add(-decrease, std::memory_order_relaxed) - decrease;
	if (val <= 0) { // Counter reach 0, move all dependents to awake list
		lock_guard_type lock(counter.m_mtx);
		while (!counter.m_dependents.empty()) {
			m_fibersAwakeList.push_now(counter.m_dependents.front());
			counter.m_dependents.pop_front();
		}
	}
	return val;
}

void LIW::LIWFiberThreadPool::ProcessTask(LIWFiberThreadPool* thisTP)
{
	LIWFiberMain* fiberMain = LIWFiberMain::InitThreadMainFiber();
	LIWFiberTask* task = nullptr;
	LIWFiberWorker* fiber = nullptr;
	while (!thisTP->m_tasks.empty() || 
		   !thisTP->m_fibersAwakeList.empty() || 
		   thisTP->m_isRunning) {
		fiber = nullptr;
		if (thisTP->m_fibersAwakeList.pop_now(fiber)) { // Acquire fiber from awake fiber list. 
			// Set fiber to perform task
			fiber->SetMainFiber(fiberMain);
			
			// Switch to fiber
			fiberMain->YieldTo(fiber);

			if (fiber->GetState() != LIWFiberState::Running) { // If fiber is not still running (meaning yielded manually), return for reuse. 
				thisTP->m_fibers.push_now(fiber);
			}
		}
		else if (thisTP->m_fibers.pop_now(fiber)) { // Acquire fiber from idle fiber list. //TODO: Currently this is spinning when empty. Make it wait. 
			if (thisTP->m_tasks.pop_now(task)) { // Acquite task
				// Set fiber to perform task
				fiber->SetMainFiber(fiberMain);
				fiber->SetRunFunction(task->m_runner, task->m_param);
				
				// Switch to fiber
				fiberMain->YieldTo(fiber);

				// Delete task, since everything was copied into call stack (fiber).
				delete task;
			}
			if (fiber->GetState() != LIWFiberState::Running) { // If fiber is not still running (meaning yielded manually), return for reuse. 
				thisTP->m_fibers.push_now(fiber);
			}
		}
	}
}
