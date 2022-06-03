#pragma once
#include "LIWThreadPoolSized.h"

class ExecutorSized
{
public:
	static LIW::LIWThreadPoolSized<1048576> pool;
};