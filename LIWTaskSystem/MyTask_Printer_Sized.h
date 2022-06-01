#pragma once
#include "LIWITask.h"
#include "Console.h"
#include <iostream>

class MyTask_Flusher_Sized :
    public LIW::LIWITask
{
public:
    void Execute(void*) override;
};

class MyTask_Printer_Sized :
    public LIW::LIWITask
{
public:
    MyTask_Printer_Sized(const std::string& str) : m_str(str) {}
    MyTask_Printer_Sized(std::string&& str) : m_str(str) {}
    void Execute(void*) override;
private:
    std::string m_str;
};
