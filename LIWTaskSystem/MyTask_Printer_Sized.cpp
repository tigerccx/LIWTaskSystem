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
