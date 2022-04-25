#pragma once
#include "LIWITask.h"
#include "Goods.h"
#include "Console.h"

#include "Executor.h"
#include "MyTask_Printer.h"

class MyTask_Worker :
    public LIW::LIWITask
{
public:
    void Execute(void*) override;
};

