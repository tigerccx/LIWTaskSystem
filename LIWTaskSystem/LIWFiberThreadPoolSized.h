#pragma once
#include <thread>
#include <functional>
#include <vector>
#include <array>

#include "LIWThreadSafeQueueSized.h"
#include "LIWFiberTask.h"
#include "LIWFiberMain.h"
#include "LIWFiberWorker.h"


namespace LIW {
	template<uint64_t FiberSize, 
			uint64_t AwakeFiberSize, 
			uint64_t TasksSize,
			uint32_t SyncCounterSize>
	class LIWFiberThreadPoolSized
	{
	public:
		typedef Util::LIWThreadSafeQueueSized<LIWFiberWorker*, FiberSize> fiber_queue_type;
		typedef std::array<LIWFiberWorker*, FiberSize> fiber_array_type;
		typedef Util::LIWThreadSafeQueueSized<LIWFiberWorker*, AwakeFiberSize> awake_fiber_queue_type;
		typedef Util::LIWThreadSafeQueueSized<LIWFiberTask*, TasksSize> task_queue_type;
		typedef typename fiber_queue_type::size_type size_type;
		typedef uint32_t counter_size_type;
		typedef int counter_type;
		typedef std::atomic<counter_type> atomic_counter_type;
		typedef std::lock_guard<std::mutex> lock_guard_type;
	private:
		struct LIWFiberSyncCounter {
			friend class LIWFiberThreadPoolSized;
		private:
			atomic_counter_type m_counter;
			std::mutex m_mtx;
			std::list<LIWFiberWorker*> m_dependents;
		};
	public:
		typedef std::array<LIWFiberSyncCounter, SyncCounterSize> sync_counter_array_type;

	public:
		LIWFiberThreadPoolSized() :
			m_isRunning(false), m_isInit(false) {}
		virtual ~LIWFiberThreadPoolSized() {}

		/// <summary>
		/// Initialize. 
		/// </summary>
		/// <param name="numWorkers"> number of workers (threads) </param>
		/// <param name="numFibers"> number of fibers (shared among threads) </param>
		void Init(int numWorkers) {
			Init(numWorkers, numWorkers);
		}
		void Init(int minWorkers, int maxWorkers) {
			//TODO: Make this adaptive somehow
			m_isRunning = true;

			for (size_type i = 0; i < FiberSize; ++i) {
				LIWFiberWorker* worker = new LIWFiberWorker(i);
				m_fibers.push_now(worker);
				m_fibersRegistered[i] = worker;
			}

			for (int i = 0; i < minWorkers; ++i) {
				m_workers.emplace_back(ProcessTask, this);
			}

			m_isInit = true;
		}

		/// <summary>
		/// Is thread pool initialized? 
		/// </summary>
		/// <returns> is initialized </returns>
		inline bool IsInit() const { return m_isInit; }
		/// <summary>
		/// Is thread pool still running? 
		/// </summary>
		/// <returns> is running </returns>
		inline bool IsRunning() const { return m_isRunning; }

		inline size_type GetTaskCount() const { return m_tasks.size(); }

		/// <summary>
		/// Submit task for the thread pool to execute. 
		/// </summary>
		/// <param name="task"> task to execute </param>
		/// <returns> is operation successful? </returns>
		inline bool Submit(LIWFiberTask* task) {
			return m_tasks.push_now(task);
		}

		/// <summary>
		/// Wait for all the submited tasked to be executed before stopping. 
		/// </summary>
		void WaitAndStop() {
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
		/// <summary>
		/// Stop after execution of currently executing tasks. Ignore others enqueued. 
		/// </summary>
		void Stop() {
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


		/*
		* Fiber task waiting
		*/
		
		/// <summary>
		/// Add a fiber worker as the dependency of a sync counter. 
		/// </summary>
		/// <param name="idxCounter"> index of the sync counter </param>
		/// <param name="worker"> dependent fiber worker </param>
		/// <returns> is operation successful? </returns>
		inline bool AddDependencyToSyncCounter(counter_size_type idxCounter, LIWFiberWorker* worker) {
			LIWFiberSyncCounter& counter = m_syncCounters[idxCounter];
			lock_guard_type lk(counter.m_mtx);
			counter.m_dependents.emplace_back(worker);
			return true;
		}
		/// <summary>
		/// Increase a sync counter. 
		/// </summary>
		/// <param name="idxCounter"> index of the sync counter </param>
		/// <param name="increase"> amount to increase (>0) </param>
		/// <returns> sync counter after increment </returns>
		inline counter_type IncreaseSyncCounter(counter_size_type idxCounter, counter_type increase = 1) {
			return m_syncCounters[idxCounter].m_counter.fetch_add(increase, std::memory_order_relaxed) + increase;
		}
		/// <summary>
		/// Decrease a sync counter (by 1).
		/// When sync counter reaches 0, the fiber worker will be awaken and put into awake list. 
		/// </summary>
		/// <param name="idxCounter"> index of the sync counter </param>
		/// <param name="decrease"> amount to decrease (>0) </param>
		/// <returns> sync counter after decrement </returns>
		counter_type DecreaseSyncCounter(counter_size_type idxCounter, counter_type decrease = 1) {
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
		/// <summary>
		/// Get the value of a sync counter. 
		/// </summary>
		/// <param name="idxCounter"> index of the sync counter </param>
		/// <returns> sync counter value </returns>
		inline counter_type GetSyncCounter(counter_size_type idxCounter) const {
			return m_syncCounters[idxCounter].m_counter.load(std::memory_order_relaxed);
		}

	private:
		// Fiber Management
		fiber_queue_type m_fibers;
		fiber_array_type m_fibersRegistered;
		// Fiber waiting Management
		awake_fiber_queue_type m_fibersAwakeList;
		sync_counter_array_type m_syncCounters;
		// Worker threads
		std::vector<std::thread> m_workers;
		// Task queue
		task_queue_type m_tasks;

	private:
		/// <summary>
		/// Loop function to process task. 
		/// </summary>
		static void ProcessTask(LIWFiberThreadPoolSized* thisTP) {
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

	private:
		bool m_isRunning;
		bool m_isInit;
	};
}



