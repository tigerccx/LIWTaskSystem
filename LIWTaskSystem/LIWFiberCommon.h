#pragma once
#include <functional>
#include <atomic>
#include <list>

namespace LIW {
	class LIWFiberMain;
	class LIWFiberWorker;

	enum class LIWFiberState {
		Uninit,
		Idle,
		Running
	};

	typedef void(*LIWFiberRunner)(LIWFiberWorker* thisFiber, void* param);
}

#define LIW_FIBER_RUNNER_DEF(function_name)