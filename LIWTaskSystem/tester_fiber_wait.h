#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

#include "FiberExecutor.h"
#include "Goods.h"

using namespace std;
using namespace LIW;
using namespace Util;

struct MyParam_MainTask {
	int mainTaskIdx;
};

struct MyParam_SubTask {
	int subTaskIdx;
	int mainTaskIdx;
	int good;
	std::atomic<int>* testCounter;
};

void MyFiberTask_SubTask(LIWFiberWorker* thisFiber, void* param) {
	MyParam_SubTask* paramST = (MyParam_SubTask*)param;
	std::cout << "SubTask [" + std::to_string(paramST->mainTaskIdx) + "-" + std::to_string(paramST->subTaskIdx) + "] proc [" + std::to_string(paramST->good) + "]\n";
	FiberExecutor::pool.DecreaseSyncCounter(paramST->mainTaskIdx);
	paramST->testCounter->fetch_add(1);
	delete paramST;
}

void MyFiberTask_MainTask(LIWFiberWorker* thisFiber, void* param) {
	MyParam_MainTask* paramMT = (MyParam_MainTask*)param;
	std::atomic<int> testCounter;

	//First Stage
	FiberExecutor::pool.AddDependencyToSyncCounter(paramMT->mainTaskIdx, thisFiber);
	testCounter.store(-100);
	FiberExecutor::pool.IncreaseSyncCounter(paramMT->mainTaskIdx, 100);
	for (int i = 0; i < 100; ++i) {
		int good = rand() % 1000;
		MyParam_SubTask* paramST = new MyParam_SubTask{ i, paramMT->mainTaskIdx, good, &testCounter };
		FiberExecutor::pool.Submit(new LIWFiberTask{ MyFiberTask_SubTask, paramST });
	}
	thisFiber->YieldToMain();
	std::cout << "MainTask 1st [" + std::to_string(paramMT->mainTaskIdx) + "] "+ std::to_string(testCounter.load()) +"\n";

	//Second Stage
	FiberExecutor::pool.AddDependencyToSyncCounter(paramMT->mainTaskIdx, thisFiber);
	testCounter.store(-100);
	FiberExecutor::pool.IncreaseSyncCounter(paramMT->mainTaskIdx, 100);
	for (int i = 0; i < 100; ++i) {
		int good = rand() % 1000;
		MyParam_SubTask* paramST = new MyParam_SubTask{ i, paramMT->mainTaskIdx, good, &testCounter };
		FiberExecutor::pool.Submit(new LIWFiberTask{ MyFiberTask_SubTask, paramST });
	}
	thisFiber->YieldToMain();
	std::cout << "MainTask 2nd [" + std::to_string(paramMT->mainTaskIdx) + "] " + std::to_string(testCounter.load()) + "\n";

	delete paramMT;
}

bool toContinue = true;

void tester_fiber_wait() {
	int countThreads = thread::hardware_concurrency();
	if (countThreads == 0)
		countThreads = 32;
	FiberExecutor::pool.Init(countThreads, 64);
	//Executor::pool.Init(32);

	ofstream fout("../../testout_fiber_wait.txt");
	cout.set_rdbuf(fout.rdbuf());

	for (int i = 0; i < 10; ++i) {
		FiberExecutor::pool.Submit(new LIWFiberTask{ MyFiberTask_MainTask, new MyParam_MainTask{i} });
	}

	Sleep(100);

	toContinue = false;

	FiberExecutor::pool.WaitAndStop();

	Goods::m_goods.block_till_empty();
	Sleep(1);
	Goods::m_goods.notify_stop();

	cout << Goods::m_goods.empty() << endl;
}