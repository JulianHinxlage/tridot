#include "ThreadPool.h"
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "ThreadPool.h"
#include "util/Clock.h"

namespace tri {

    ThreadPool::ThreadPool(){
        nextTaskId = 0;
        nextThreadId = 0;
        nextGroupId = 0;
    }

    int ThreadPool::addTaskGroup() {
        groups.emplace_back();
        TaskGroup& group = groups.back();
        group.groupId = nextGroupId++;
        group.pending = true;
        return group.groupId;
    }

    void ThreadPool::setTaskGroupOrder(const std::vector<int>& list) {
        int lastGroupId = 0;
        bool first = true;
        for (auto& groupId : list) {
            if (!first) {
                for (auto& group : groups) {
                    if (group.groupId == groupId) {
                        group.dependencies.push_back(lastGroupId);
                        break;
                    }
                }
            }
            first = false;
            lastGroupId = groupId;
        }
    }

    void ThreadPool::removeTaskGroup(int groupId) {
        for (int i = 0; i < groups.size(); i++) {
            if (groups[i].groupId == groupId) {
                groups.erase(groups.begin() + i);
                break;
            }
        }
    }

    int ThreadPool::addTask(const std::function<void()>& function, int groupId) {
        taskMutex.lock();
        tasks.emplace_back();
        int taskId = nextTaskId++;
        Task &task = tasks.back();
        task.taskId = taskId;
        task.groupId = groupId;
        task.function = function;
        task.state = Task::State::PENDING;

        for (auto& group : groups) {
            if (group.groupId == groupId) {
                if (!group.pending) {
                    task.state = Task::State::BLOCKED;
                    break;
                }
            }
        }

        taskMutex.unlock();
        for (auto& worker : workers) {
            if (worker->taskId == -1) {
                worker->condition.notify_one();
                break;
            }
        }
        return taskId;
    }

    void ThreadPool::joinTask(int taskId) {
        std::unique_lock<std::mutex> lock(taskMutex);
        bool found = true;
        while (found) {
            found = false;
            for (auto& task : tasks) {
                if (task.taskId == taskId) {
                    found = true;
                    break;
                }
            }
            if (found) {
                taskCondition.wait(lock);
            }
        }
    }

    void ThreadPool::joinAllTasks(){
        std::unique_lock<std::mutex> lock(taskMutex);
        while (tasks.size() > 0) {
            taskCondition.wait(lock);
        }
    }

    int ThreadPool::addThread(const std::function<void()>& function) {
        threads.emplace_back();
        Thread& thread = threads.back();
        thread.threadId = nextThreadId++;
        thread.run(function);
        return thread.threadId;
    }

    void ThreadPool::jointThread(int threadId) {
        for (auto& thread : threads) {
            if (thread.threadId == threadId) {
                thread.join();
                break;
            }
        }
    }

    void ThreadPool::terminateThread(int threadId) {
        for (int i = 0; i < threads.size(); i++) {
            if (threads[i].threadId == threadId) {
                threads[i].terminate();
                threads.erase(threads.begin() + i);
                break;
            }
        }
    }

    void ThreadPool::setPoolSize(int threadCount) {
        if (threadCount > workers.size()) {
            int oldCount = (int)workers.size();
            workers.resize(threadCount);
            for (int i = oldCount; i < threadCount; i++) {
                workers[i] = std::make_shared<Worker>();
                workers[i]->pool = this;
                workers[i]->run();
            }
        } else if (threadCount < workers.size() && threadCount >= 0) {
            for (int i = threadCount; i < workers.size(); i++) {
                workers[i]->join();
            }
            workers.resize(threadCount);
        }
    }

    void ThreadPool::startup() {
        setPoolSize(std::thread::hardware_concurrency());
    }

    void ThreadPool::update() {

    }

    void ThreadPool::shutdown() {
        for (int i = 0; i < workers.size(); i++) {
            workers[i]->join();
        }
        workers.clear();
        tasks.clear();
        for (int i = 0; i < threads.size(); i++) {
            threads[i].terminate();
        }
        threads.clear();
    }

    void ThreadPool::Worker::run(){
        running = true;
        taskId = -1;
        threadId = pool->addThread([this]() {
            while (running) {

                std::function<void()> function;
                pool->taskMutex.lock();
                for (int i = 0; i < pool->tasks.size(); i++) {
                    Task& task = pool->tasks[i];
                    if (task.state == Task::State::PENDING) {
                        taskId = task.taskId;
                        task.state = Task::State::RUNNING;
                        function = task.function;
                        break;
                    }
                }
                pool->taskMutex.unlock();

                if (taskId != -1) {
                    if (function) {
                        function();
                        function = nullptr;
                    }

                    pool->taskMutex.lock();
                    for (int i = 0; i < pool->tasks.size(); i++) {
                        Task& task = pool->tasks[i];
                        if (task.taskId == taskId) {
                            pool->tasks.erase(pool->tasks.begin() + i);
                            break;
                        }
                    }
                    taskId = -1;
                    pool->taskCondition.notify_one();
                    pool->taskMutex.unlock();
                }
                else {
                    std::unique_lock<std::mutex> lock(mutex);
                    condition.wait(lock);
                }
            }
        });
    }

    void ThreadPool::Worker::join(){
        running = false;
        condition.notify_all();
        pool->jointThread(threadId);
    }

    void ThreadPool::Thread::run(const std::function<void()>& function){
        if (thread) {
            terminate();
        }
        thread = std::make_shared<std::thread>(function);
    }

    void ThreadPool::Thread::join(){
        if (thread) {
            if (thread->joinable()) {
                thread->join();
            }
        }
    }

    void ThreadPool::Thread::terminate(){
        if (thread) {
            if (thread->joinable()) {
                thread->detach();
            }
            thread = nullptr;
        }
    }

}
