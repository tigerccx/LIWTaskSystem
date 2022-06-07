#pragma once
#include <thread>
#include <functional>
#include <vector>
#include <array>

#include "LIWThreadSafeQueue.h"
#include "LIWFiberTask.h"
#include "LIWFiberMain.h"
#include "LIWFiberWorker.h"


namespace LIW {
	class LIWFiberThreadPool
	{
	public:
		typedef Util::LIWThreadSafeQueue<LIWFiberWorker*>::size_type size_type;
		typedef uint32_t counter_size_type;
		typedef int counter_type;
		typedef std::atomic<counter_type> atomic_counter_type;
		typedef std::lock_guard<std::mutex> lock_guard_type;
	private:
		struct LIWFiberSyncCounter {
			friend class LIWFiberThreadPool;
		private:
			atomic_counter_type m_counter;
			std::mutex m_mtx;
			std::list<LIWFiberWorker*> m_dependents;
		};
	public:
		LIWFiberThreadPool();
		virtual ~LIWFiberThreadPool();

		/// <summary>
		/// Initialize. 
		/// </summary>
		/// <param name="numWorkers"> number of workers (threads) </param>
		/// <param name="numFibers"> number of fibers (shared among threads) </param>
		void Init(int numWorkers, int numFibers);
		void Init(int minWorkers, int maxWorkers, int minFibers, int maxFibers);

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
			m_tasks.push(task);
			return true;
		}

		/// <summary>
		/// Wait for all the submited tasked to be executed before stopping. 
		/// </summary>
		void WaitAndStop();
		/// <summary>
		/// Stop after execution of currently executing tasks. Ignore others enqueued. 
		/// </summary>
		void Stop();


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
		counter_type DecreaseSyncCounter(counter_size_type idxCounter, counter_type decrease = 1);
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
		Util::LIWThreadSafeQueue<LIWFiberWorker*> m_fibers;
		std::vector<LIWFiberWorker*> m_fibersRegistered;
		// Fiber waiting Management
		Util::LIWThreadSafeQueue<LIWFiberWorker*> m_fibersAwakeList;
		std::array<LIWFiberSyncCounter, 1024> m_syncCounters;
		// Worker threads
		std::vector<std::thread> m_workers;
		// Task queue
		Util::LIWThreadSafeQueue<LIWFiberTask*> m_tasks;

	private:
		/// <summary>
		/// Loop function to process task. 
		/// </summary>
		static void ProcessTask(LIWFiberThreadPool* thisTP);

	private:
		bool m_isRunning;
		bool m_isInit;
	};
}



