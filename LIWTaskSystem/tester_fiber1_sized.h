#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

#include "FiberExecutorSized.h"
#include "Goods.h"

using namespace std;
using namespace LIW;
using namespace Util;

void MyFiberTask_Printer(LIWFiberWorker* thisFiber, void* param) {
	string* str = reinterpret_cast<string*>(param);
	std::cout << *str;
	delete str;
}

void MyFiberTask_Worker(LIWFiberWorker* thisFiber, void* param) {
	int good = rand() % 1000;
	Goods::m_goods.push_now(good);
	string* strout = new string("Worker putting [" + std::to_string(good) + "]\n");
	FiberExecutorSized::pool.Submit(new LIWFiberTask{ MyFiberTask_Printer,  strout });
}

void MyFiberTask_Consumer(LIWFiberWorker* thisFiber, void* param) {
	int good;
	Goods::m_goods.pop(good); //TODO: Thread pool could end up waiting on this. And no further task will be performed. 
	string* strout = new string("Consumer acquiring [" + std::to_string(good) + "]\n");
	FiberExecutorSized::pool.Submit(new LIWFiberTask{ MyFiberTask_Printer,  strout });
}

bool toContinue = true;

void Run() {
	while (toContinue) {
		if (FiberExecutorSized::pool.GetTaskCount() < 32768) {
			FiberExecutorSized::pool.Submit(new LIWFiberTask{ MyFiberTask_Worker });
			FiberExecutorSized::pool.Submit(new LIWFiberTask{ MyFiberTask_Consumer });
		}
	}
}

void tester_fiber1_sized() {
	int countThreads = thread::hardware_concurrency();
	if (countThreads == 0)
		countThreads = 32;
	FiberExecutorSized::pool.Init(countThreads, 64);
	//Executor::pool.Init(32);

	ofstream fout("../../testout_fiber1_sized.txt");
	cout.set_rdbuf(fout.rdbuf());

	thread threadTest(Run);

	Sleep(5 * 1000);

	toContinue = false;

	threadTest.join();

	FiberExecutorSized::pool.WaitAndStop();

	Goods::m_goods.block_till_empty();
	Sleep(1);
	Goods::m_goods.notify_stop();

	cout << Goods::m_goods.empty() << endl;
}