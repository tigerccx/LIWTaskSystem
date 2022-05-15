#pragma once
#include <thread>
#include <functional>
#include <vector>

#include "LIWThreadSafeQueue.h"
#include "LIWFiberTask.h"
#include "LIWFiberMain.h"
#include "LIWFiberWorker.h"


namespace LIW {
	class LIWFiberThreadPool
	{
	public:
		typedef Util::LIWThreadSafeQueue<LIWFiberWorker*>::size_type size_type;
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
		/// <returns></returns>
		bool Submit(LIWFiberTask* task);

		/// <summary>
		/// Wait for all the submited tasked to be executed before stopping. 
		/// </summary>
		void WaitAndStop();
		/// <summary>
		/// Stop after execution of currently executing tasks. Ignore others enqueued. 
		/// </summary>
		void Stop();

	private:
		Util::LIWThreadSafeQueue<LIWFiberWorker*> m_fibers;
		std::vector<LIWFiberWorker*> m_fibersRegistered;
		//Util::LIWThreadSafeQueue<LIWFiberTask*> m_fibersWaitList;
		//TODO: Add waiting
		std::vector<std::thread> m_workers;
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



