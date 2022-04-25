#include "MyTask_Consumer.h"

void MyTask_Consumer::Execute(void*)
{
	int good;
	Goods::m_goods.pop(good); //TODO: Thread pool could end up waiting on this. And no further task will be performed. 
	Executor::pool.Submit(new MyTask_Printer("Consumer acquiring [" + std::to_string(good) + "]\n"));
}
