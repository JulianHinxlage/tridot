//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "pch.h"
#include "System.h"

namespace tri {

    class ThreadPool : public System {
    public:
        ThreadPool();

        int addTaskGroup();
        void setTaskGroupOrder(const std::vector<int> &list);
        void removeTaskGroup(int groupId);

        int addTask(const std::function<void()>& function, int groupId = -1);
        void joinTask(int taskId);
        void joinAllTasks();

        int addThread(const std::function<void()>& function);
        void jointThread(int threadId);
        void terminateThread(int threadId);

        void setPoolSize(int threadCount);

        void startup() override;
        void update() override;
        void shutdown() override;

    private:
        class Thread {
        public:
            std::shared_ptr<std::thread> thread;
            int threadId;

            void run(const std::function<void()>& function);
            void join();
            void terminate();
        };

        class Worker {
        public:
            int threadId;
            bool running;
            int taskId;
            ThreadPool* pool;
            std::mutex mutex;
            std::condition_variable condition;
            void run();
            void join();
        };

        class Task {
        public:
            std::function<void()> function;
            int taskId;
            int groupId;

            enum State {
                BLOCKED,
                PENDING,
                RUNNING,
            };

            State state;
        };

        class TaskGroup {
        public:
            int groupId;
            bool pending;
            std::vector<int> dependencies;
        };

        std::vector<Thread> threads;
        std::vector<std::shared_ptr<Worker>> workers;
        std::vector<Task> tasks;
        std::vector<TaskGroup> groups;
        std::mutex taskMutex;
        std::condition_variable taskCondition;
        int nextTaskId;
        int nextThreadId;
        int nextGroupId;
    };

}
