#include "MyTask_Worker_Sized.h"

void MyTask_Worker_Sized::Execute(void*)
{
	int good = rand() % 1000;
	Goods::m_goods.push_now(good);
	//auto taskPrint = new MyTask_Printer_Sized("Worker putting [" + std::to_string(good) + "]\n");
	auto taskPrint = new MyTask_SpecializedPrinter_Sized(good, true);
	ExecutorSized::pool.Submit(taskPrint);
}
