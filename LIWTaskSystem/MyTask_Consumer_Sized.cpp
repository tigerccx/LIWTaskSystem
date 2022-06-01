#include "MyTask_Consumer_Sized.h"

void MyTask_Consumer_Sized::Execute(void*)
{
	int good;
	Goods::m_goods.pop(good); //TODO: Thread pool could end up waiting on this. And no further task will be performed. 
	auto taskPrint = new MyTask_Printer_Sized("Consumer acquiring [" + std::to_string(good) + "]\n");
	while (!ExecutorSized::pool.SubmitNow(taskPrint)) {
		std::this_thread::yield();
	}
}
