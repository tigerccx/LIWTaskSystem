#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <string>

#include <windows.h>

#include "LIWThreadSafeQueue.h"

using namespace std;
using namespace LIW;
using namespace Util;

LIWThreadSafeQueue<int> queue_;
LIWThreadSafeQueue<string> queue_outputs_;

bool toContinue = true;
size_t GOODS_QUEUE_SIZE = 1024;
size_t PRINT_QUEUE_SIZE = 4096;

void Worker(int idx) {
	while (toContinue) {
		int count = 0;
		for (int i = 0; i < 100; ++i) {
			count += rand() % 100;
		}
		while (queue_.size() > GOODS_QUEUE_SIZE) { std::this_thread::yield(); }
		queue_.push_now(count);
		while (queue_outputs_.size() > PRINT_QUEUE_SIZE) { std::this_thread::yield(); }
		queue_outputs_.push_now("Worker[" + to_string(idx) + "] putting [" + to_string(count) + "] in queue.\n");
		//this_thread::sleep_for(100ms);
	}
}

void Consumer(int idx) {
	while (!queue_.empty() || toContinue)
	{
		int count;
		//if (queue_.pop_now(count) < 0) {
		//	cout << "Consumer[" << idx << "] taking failed." << endl;
		//}
		//else
		//{
		//	cout << "Consumer[" << idx << "] taking [" << count << "] from queue." << endl;
		//}
		queue_.pop(count);
		//lock_guard<mutex> lckout(mtxOutput);
		while (queue_outputs_.size() > PRINT_QUEUE_SIZE) { std::this_thread::yield(); }
		queue_outputs_.push_now("Consumer[" + to_string(idx) + "] taking [" + to_string(count) + "] from queue.\n");
	}
}

void Print() {
	string output;
	while (!queue_outputs_.empty() || toContinue) {
		queue_outputs_.pop(output);
		cout << output;
		flush(cout);
	}
}

//
// Thread-safe Queue Tester
//
void tester0_sizefree() {
	vector<thread> threadWorkers;
	vector<thread> threadConsumers;
	vector<thread> threadPrinters;

	ofstream fout("../../testout0_sizefree.txt");
	cout.set_rdbuf(fout.rdbuf());

	for (int i = 0; i < 32; ++i) {
		threadWorkers.emplace_back(thread(Worker, i));
	}
	for (int i = 0; i < 32; ++i) {
		threadConsumers.emplace_back(thread(Consumer, i));
	}
	for (int i = 0; i < 64; ++i) {
		threadPrinters.emplace_back(thread(Print));
	}

	Sleep(20 * 1000);

	toContinue = false;
	for (int i = 0; i < threadWorkers.size(); ++i) {
		threadWorkers[i].join();
	}

	queue_.block_till_empty();
	Sleep(1);
	queue_.notify_stop();

	for (int i = 0; i < threadConsumers.size(); ++i) {
		threadConsumers[i].join();
	}

	queue_outputs_.block_till_empty();
	Sleep(1);
	queue_outputs_.notify_stop();

	for (int i = 0; i < threadPrinters.size(); ++i) {
		threadPrinters[i].join();
	}
	cout << queue_.empty() << endl;
};