#pragma once
#include <thread>
#include <functional>
#include <vector>

#include "LIWThreadSafeQueue.h"
#include "LIWITask.h"

namespace LIW {
	class LIWThreadPool
	{
	public:
		LIWThreadPool();
		virtual ~LIWThreadPool();

		/// <summary>
		/// Initialize. 
		/// </summary>
		/// <param name="numWorkers"> number of workers (threads) </param>
		void Init(int numWorkers);
		/// <summary>
		/// Initialize. 
		/// </summary>
		/// <param name="minWorkers"> number of minimum workers </param>
		/// <param name="maxWorkers"> number of maximum workers </param>
		void Init(int minWorkers, int maxWorkers);

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
		/// Submit task for the thread pool to execute. 
		/// </summary>
		/// <param name="task"> task to execute </param>
		/// <returns></returns>
		bool Submit(LIWITask* task);

		/// <summary>
		/// Wait for all the submited tasked to be executed before stopping. 
		/// </summary>
		void WaitAndStop();
		/// <summary>
		/// Stop after execution of currently executing tasks. Ignore others enqueued. 
		/// </summary>
		void Stop();

	private:
		std::vector<std::thread> m_workers;
		Util::LIWThreadSafeQueue<LIWITask*> m_tasks;


	private:
		/// <summary>
		/// Loop function to process task. 
		/// </summary>
		void ProcessTask();

	private:
		bool __m_isRunning;
		bool __m_isInit;
	};
}


