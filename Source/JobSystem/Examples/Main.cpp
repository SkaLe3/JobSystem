// main.cpp

#include "Jobs/JobSystem.h"
#include "Platform/Platform.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace SV;

void Example_IndependentTasks()
{
	std::cout << "\n=== Example 1: Independent Tasks ===\n";

	std::shared_ptr<TaskEvent> taskA = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Task A executing\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "Task A complete\n";
		});
	std::shared_ptr<TaskEvent> taskB = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Task B executing\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "Task B complete\n";
		});
	std::shared_ptr<TaskEvent> taskC = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Task C executing\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "Task C complete\n";
		});

	taskA->Wait();
	taskB->Wait();
	taskC->Wait();

	std::cout << "All independent tasks completed\n";
}

void Example_TaskChain()
{
	std::cout << "\n=== Example 2: Task Chain ===\n";

	std::shared_ptr<TaskEvent> task1 = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Task 1: Loading resources...\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "Task 1: Resources loaded\n";
		});

	std::shared_ptr<TaskEvent> task2 = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Task 2: Processing data...\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "Task 2: Data processed\n";
		}, task1);

	std::shared_ptr<TaskEvent> task3 = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Task 3: Finalizing...\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::cout << "Task 3: Complete\n";
		}, task2);

	task3->Wait();
	std::cout << "Task chain completed\n";
}

void Example_ForkJoin()
{
	std::cout << "\n=== Example 3: Fork-Join Pattern ===\n";

	std::shared_ptr<TaskEvent> taskA = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Parallel Task A started\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			std::cout << "Parallel Task A finished\n";
		});
	std::shared_ptr<TaskEvent> taskB = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Parallel Task B started\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
			std::cout << "Parallel Task B finished\n";
		});
	std::shared_ptr<TaskEvent> taskC = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Parallel Task C started\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(80));
			std::cout << "Parallel Task C finished\n";
		});

	std::vector<std::shared_ptr<TaskEvent>> prerequisites = { taskA, taskB, taskC };

	std::shared_ptr<TaskEvent> joinTask = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Join task: All parallel tasks completed, continuing...\n";
		}, prerequisites);

	joinTask->Wait();
	std::cout << "Fork-join pattern completed\n";
}

void Example_NestedTasks()
{
	std::cout << "\n=== Example 4: Nested Task Spawning ===\n";

	std::shared_ptr<TaskEvent> parentTask = JobTask::CreateAndDispatch(
		[]()
		{
			std::cout << "Parent task started\n";

			std::shared_ptr<TaskEvent> childA = JobTask::CreateAndDispatch(
				[]()
				{
					std::cout << "	Child A executing\n";
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				});
			std::shared_ptr<TaskEvent> childB = JobTask::CreateAndDispatch(
				[]()
				{
					std::cout << "	Child B executing\n";
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				});

			childA->Wait();
			childB->Wait();

			std::cout << "Parent task completed (after children)\n";
		});

	parentTask->Wait();
	std::cout << "Nested tasks completed\n";
}

void Example_ParallelProcessing()
{
	std::cout << "\n=== Examples 5: Parallel Data Processing ===\n";

	constexpr int32_t NUM_CHUNKS = 8;
	std::vector<std::shared_ptr<TaskEvent>> processingTasks;
	std::vector<int> results(NUM_CHUNKS, 0);

	for (int32_t i = 0; i < NUM_CHUNKS; ++i)
	{
		std::shared_ptr<TaskEvent> task = JobTask::CreateAndDispatch(
			[i, &results]()
			{
				// Simulate expensive computation
				int32_t sum = 0;
				for (int32_t j = 0; j < 1000000; ++j)
				{
					sum += j * (i + 1);
				}
				results[i] = sum % 10000;
				std::cout << "Chunk " << i << " processed: " << results[i] << "\n";
			});
		processingTasks.push_back(task);
	}

	std::shared_ptr<TaskEvent> aggregateTask = JobTask::CreateAndDispatch(
		[&results]()
		{
			int32_t total = 0;
			for (int32_t result : results)
			{
				total += result;
			}
			std::cout << "Total aggregate: " << total << "\n";
		}, processingTasks);

	aggregateTask->Wait();
	std::cout << "Parallel processing completed\n";
}


int main()
{
	std::cout << "=== Job System Examples ===\n";
	std::cout << "Logical cores: " << Platform::GetLogicalCoreCount() << "\n";

	JobSystem::Initialize();

	Example_IndependentTasks();
	Example_TaskChain();
	Example_ForkJoin();
	Example_NestedTasks();
	Example_ParallelProcessing();

	std::cout << "\n=== All Examples Completed ===\n";
	std::cout << "Waiting before shutdown...\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	JobSystem::Shutdown();

	std::cout << "Program finished\n";
	return 0;
}