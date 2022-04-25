#pragma once
#include "LIWITask.h"
#include "Console.h"
#include <iostream>

class MyTask_Flusher :
    public LIW::LIWITask
{
public:
    void Execute(void*) override;
};

class MyTask_Printer :
    public LIW::LIWITask
{
public:
    MyTask_Printer(const std::string& str) : m_str(str) {}
    MyTask_Printer(std::string&& str) : m_str(str) {}
    void Execute(void*) override;
private:
    std::string m_str;
};
