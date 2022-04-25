#include "MyTask_Worker.h"

void MyTask_Worker::Execute(void*)
{
	int good = rand() % 1000;
	Goods::m_goods.push(good);
	Executor::pool.Submit(new MyTask_Printer("Worker putting [" + std::to_string(good) + "]\n"));
}
