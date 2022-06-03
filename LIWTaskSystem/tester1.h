#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

#include "ExecutorSized.h"
#include "MyTask_Worker_Sized.h"
#include "MyTask_Consumer_Sized.h"
#include "MyTask_Printer_Sized.h"

using namespace std;
using namespace LIW;
using namespace Util;

bool toContinue = true;

void Run() {
	while (toContinue) {
		if (ExecutorSized::pool.GetTasksCount() + 128 < ExecutorSized::pool.GetCapacity()) {
			ExecutorSized::pool.Submit(new MyTask_Worker_Sized());
			ExecutorSized::pool.Submit(new MyTask_Consumer_Sized());
		}
	}
	cout << "Stop Running" << endl;
}

void tester1() {
	int countThreads = thread::hardware_concurrency();
	if (countThreads == 0)
		countThreads = 32;
	ExecutorSized::pool.Init(countThreads);
	//Executor::pool.Init(32);

	ofstream fout("../../testout1.txt");
	cout.set_rdbuf(fout.rdbuf());

	thread threadTest(Run);

	Sleep(5 * 1000);

	toContinue = false;

	threadTest.join();

	ExecutorSized::pool.WaitAndStop();

	Goods::m_goods.block_till_empty();
	Sleep(1);
	Goods::m_goods.notify_stop();

	cout << Goods::m_goods.empty() << endl;
}