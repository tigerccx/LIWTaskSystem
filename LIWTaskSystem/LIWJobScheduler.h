#pragma once
#include "LIWThreadSafeQueue.h"


namespace LIW {
	class LIWJobScheduler
	{
	public:
		typedef Util::LIWThreadSafeQueue<LIWFiber*> fiberQueue_type;
	public:
		LIWFiberScheduler(fiberQueue_type& fiberQueue);
		virtual ~LIWFiberScheduler() = default;
		LIWFiberScheduler(const LIWFiberScheduler& other) = delete;
		LIWFiberScheduler(LIWFiberScheduler&& other) = delete;
		LIWFiberScheduler& operator=(const LIWFiberScheduler& other) = delete;
		LIWFiberScheduler& operator=(LIWFiberScheduler&& other) = delete;

		LIWFiber* FetchNextFiber();
	private:
		fiberQueue_type& m_fiberQueue;
	};
}


