#pragma once
#include "LIWFiberThreadPoolSized.h"

class FiberExecutorSized
{
public:
	typedef LIW::LIWFiberThreadPoolSized<1<<8, 1<<10, 1<<16, 1<<10> fiber_pool_type;
	static fiber_pool_type pool;
};