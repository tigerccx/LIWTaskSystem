#include "MyTask_Printer.h"

void MyTask_Flusher::Execute(void*)
{
	std::string str;
	while (Console::m_outputs.pop_now(str) >= 0) {
		std::cout << str;
	}
}

void MyTask_Printer::Execute(void*)
{
	std::cout << m_str;
}
