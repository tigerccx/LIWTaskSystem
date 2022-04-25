#pragma once
#include "LIWThreadSafeQueue.h"

class Goods
{
public:
	static LIW::Util::LIWThreadSafeQueue<int> m_goods;
};

