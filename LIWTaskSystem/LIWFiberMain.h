#pragma once
#include "LIWFiberCommon.h"

namespace LIW {
//
// Win32
//
#ifdef _WIN32
#include <windows.h>

	class LIWFiberMain final
	{
		friend class LIWFiberWorker;
	public:
		friend class LIWFiberWorker;
	public:
		static inline LIWFiberMain* InitThreadMainFiber() {
			return new LIWFiberMain();
		}
		void YieldTo(LIWFiberWorker* fiberOther);
	private:
		LIWFiberMain();
		~LIWFiberMain();
	private:
		LPVOID m_sysFiber;
	};
#endif 
}

