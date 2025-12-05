// main.cpp
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "TaskDispatcher.h"

using namespace SV;




int main()
{
	TaskDispatcher::Init();

	std::shared_ptr<TaskEvent> preTask = Task::CreateAndDispatch(
		[]()
		{
			std::cout << "start preTask \n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "finish preTask\n";
		});

	std::shared_ptr<TaskEvent> nextTask = Task::CreateAndDispatch(
		[]()
		{
			std::cout << "start nextTask\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			std::shared_ptr<TaskEvent> taskA = Task::CreateAndDispatch([]() {std::cout << "Task A\n"; });
			std::shared_ptr<TaskEvent> taskB = Task::CreateAndDispatch([]() {std::cout << "Task B\n"; });

			std::vector<std::shared_ptr<TaskEvent>> prerequisites;
			std::shared_ptr<TaskEvent> taskC = Task::CreateAndDispatch([]() {std::cout << "Task C\n"; }, prerequisites);

			std::cout << "finish nextTask\n";
		},
		preTask);

	std::shared_ptr<TaskEvent> aloneTask = Task::CreateAndDispatch(
		[]()
		{
			std::cout << "start aloneTask\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "finish aloneTask\n";
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	return 0;
}