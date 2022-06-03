#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

#include "Executor.h"
#include "MyTask_Worker.h"
#include "MyTask_Consumer.h"
#include "MyTask_Printer.h"

using namespace std;
using namespace LIW;
using namespace Util;

bool toContinue = true;

void Run() {
	while (toContinue) {
		if (Executor::pool.GetTasksCount() < 32768) {
			Executor::pool.Submit(new MyTask_Worker());
			Executor::pool.Submit(new MyTask_Consumer());
		}
	}
}

void tester1_sizefree() {
	int countThreads = thread::hardware_concurrency();
	if (countThreads == 0)
		countThreads = 32;
	Executor::pool.Init(countThreads);
	//Executor::pool.Init(32);

	ofstream fout("../../testout1_sizefree.txt");
	cout.set_rdbuf(fout.rdbuf());

	thread threadTest(Run);

	Sleep(5 * 1000);

	toContinue = false;

	threadTest.join();

	Executor::pool.WaitAndStop();

	Goods::m_goods.block_till_empty();
	Sleep(1);
	Goods::m_goods.notify_stop();

	cout << Goods::m_goods.empty() << endl;
}