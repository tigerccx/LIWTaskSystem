#pragma once
#include "LIWThreadPoolSized.h"

class ExecutorSized
{
public:
	static LIW::LIWThreadPoolSized<32768> pool;
};