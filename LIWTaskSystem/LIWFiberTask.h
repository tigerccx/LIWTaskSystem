#pragma once
#include "LIWFiberCommon.h"

namespace LIW {
	struct LIWFiberTask {
		LIWFiberRunner m_runner = nullptr;
		void* m_param = nullptr;
	};
}