#pragma once
#include <string>

#include "LIWThreadSafeQueue.h"

class Console
{
public:
	static LIW::Util::LIWThreadSafeQueue<std::string> m_outputs;
};

