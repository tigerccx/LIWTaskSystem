#pragma once
#include "LIWFiberCommon.h"

/*
* Here is a description of the standard API of LIWFiberWorker.
* All platform-specific implementation should follow this api layout.
*/
/*

*/

namespace LIW {
//
// Win32
//
#ifdef _WIN32
#include <windows.h>

	class LIWFiberWorker final
	{
		friend class LIWFiberMain;
	public:
		LIWFiberWorker();
		LIWFiberWorker(int id);
		~LIWFiberWorker();
		LIWFiberWorker(const LIWFiberWorker& other) = delete;
		LIWFiberWorker(LIWFiberWorker&& other) = default;
		LIWFiberWorker& operator=(const LIWFiberWorker& other) = delete;
		LIWFiberWorker& operator=(LIWFiberWorker&& other) = default;

		//Set the run function for fiber
		inline void SetRunFunction(LIWFiberRunner runFunc, void* param = nullptr) {
			m_runFunction = runFunc;
			m_param = param;
			m_state = LIWFiberState::Idle;
		}
		//Set the main fiber when obtained by another fiber
		inline void SetMainFiber(LIWFiberMain* fiberMain) { m_fiberMain = fiberMain; }
		//A loop which will yield after finishing runnning a run function
		inline void Run() {
			while (m_isRunning) {
				m_state = LIWFiberState::Running;
				m_runFunction(this, m_param);
				m_state = LIWFiberState::Idle;
				YieldToMain();
			}
		}
		//Stop worker fiber
		inline void Stop() {
			m_isRunning = false;
		}
		//Yield
		inline void YieldToMain();
		//Yield to a specific fiber
		inline void YieldTo(LIWFiberWorker* fiberYieldTo) {
			SwitchToFiber(fiberYieldTo->m_sysFiber);
		}
		//Get current state of the fiber
		inline LIWFiberState GetState() const { return m_state; } 

	private:
		LIWFiberState m_state = LIWFiberState::Uninit; // State of this fiber
		LIWFiberRunner m_runFunction = nullptr; // Running function of this fiber
		void* m_param = nullptr; // Parameters for the running function
		LIWFiberMain* m_fiberMain = nullptr; // Current main fiber of the thread this fiber is running on
		int m_id = -1; // ID of the fiber
		bool m_isRunning = true; // Is this fiber still running? (Has it not been terminated?) 

	private:
		static void __stdcall InternalFiberRun(LPVOID param) {
			LIWFiberWorker* thisFiber = reinterpret_cast<LIWFiberWorker*>(param);
			thisFiber->Run();
		}
	private:
		LPVOID m_sysFiber;
	};

#endif 
}



