#pragma once
#include "LIWITask.h"
#include "Goods.h"
#include "Console.h"

#include "ExecutorSized.h"
#include "MyTask_Printer_Sized.h"

class MyTask_Consumer_Sized :
    public LIW::LIWITask
{
public:
    void Execute(void*) override;
};

