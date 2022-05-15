#pragma once
#include <functional>

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