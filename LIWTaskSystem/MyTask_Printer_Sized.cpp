#include "MyTask_Printer_Sized.h"

void MyTask_Flusher_Sized::Execute(void*)
{
	std::string str;
	while (Console::m_outputs.pop_now(str)) {
		std::cout << str;
	}
}

void MyTask_Printer_Sized::Execute(void*)
{
	std::cout << m_str;
}

void MyTask_SpecializedPrinter_Sized::Execute(void*)
{
	if (m_isWorker) {
		std::cout << "Worker putting [" + std::to_string(m_val) + "]\n";
	}
	else
	{
		std::cout << "Consumer acquiring [" + std::to_string(m_val) + "]\n";
	}
}
