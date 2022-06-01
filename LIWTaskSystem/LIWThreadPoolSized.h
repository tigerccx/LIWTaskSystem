#pragma once
#include <thread>
#include <functional>
#include <vector>

#include "LIWThreadSafeQueueSized.h"
#include "LIWITask.h"

namespace LIW {
	template<uint64_t TasksSize>
	class LIWThreadPoolSized
	{
	public:
		LIWThreadPoolSized() : __m_isRunning(false), __m_isInit(false) {}
		virtual ~LIWThreadPoolSized() {}

		/// <summary>
		/// Initialize. 
		/// </summary>
		/// <param name="numWorkers"> number of workers (threads) </param>
		void Init(int numWorkers) {
			Init(numWorkers, numWorkers);
		}
		/// <summary>
		/// Initialize. 
		/// </summary>
		/// <param name="minWorkers"> number of minimum workers </param>
		/// <param name="maxWorkers"> number of maximum workers </param>
		void Init(int minWorkers, int maxWorkers) {
			__m_isRunning = true;
			for (int i = 0; i < minWorkers; ++i) {
				m_workers.emplace_back(std::thread(std::bind(&LIW::LIWThreadPoolSized<TasksSize>::ProcessTask, this)));
			}
			__m_isInit = true;
		}

		/// <summary>
		/// Is thread pool initialized? 
		/// </summary>
		/// <returns> is initialized </returns>
		inline bool IsInit() const { return __m_isInit; }
		/// <summary>
		/// Is thread pool still running? 
		/// </summary>
		/// <returns> is running </returns>
		inline bool IsRunning() const { return __m_isRunning; }

		/// <summary>
		/// Submit task for the thread pool to execute when task queue is not full. 
		/// </summary>
		/// <param name="task"> task to execute </param>
		/// <returns></returns>
		inline bool Submit(LIWITask* task) {
			return m_tasks.push(task);
		}
		/// <summary>
		/// Submit task for the thread pool to execute immediately. 
		/// </summary>
		/// <param name="task"> task to execute </param>
		/// <returns></returns>
		inline bool SubmitNow(LIWITask* task) {
			return m_tasks.push_now(task);
		}

		/// <summary>
		/// Wait for all the submited tasked to be executed before stopping. 
		/// </summary>
		void WaitAndStop() {
			__m_isRunning = false;
			m_tasks.block_till_empty();
			using namespace std::chrono;
			std::this_thread::sleep_for(1ms);
			m_tasks.notify_stop();

			for (int i = 0; i < m_workers.size(); ++i) {
				m_workers[i].join();
			}
		}
		/// <summary>
		/// Stop after execution of currently executing tasks. Ignore others enqueued. 
		/// </summary>
		void Stop() {
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

	private:
		std::vector<std::thread> m_workers;
		Util::LIWThreadSafeQueueSized<LIWITask*, TasksSize> m_tasks;


	private:
		/// <summary>
		/// Loop function to process task. 
		/// </summary>
		void ProcessTask() {
			while (!m_tasks.empty() || __m_isRunning) {
				LIWITask* task = nullptr;
				if (m_tasks.pop(task)) {
					task->Execute(nullptr);
					delete task;
				}
			}
		}

	private:
		bool __m_isRunning;
		bool __m_isInit;
	};
}


