//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include <barrier>

namespace tri {

	class RenderThread : public System {
	public:
		bool useDedicatedThread;
		bool executeRenderPipeline;

		RenderThread();
		void update() override;
		void synchronize();
		void lock();
		void unlock();
		void addTask(const std::function<void()>& task);
		void launch(const std::function<void()>& init = nullptr);
		void terminate();

	private:
		std::thread* thread;
		std::mutex mutex;
		std::atomic_bool running;
		std::barrier<> barrier;

		std::vector<std::function<void()>> tasks;
		std::vector<std::function<void()>> currentTasks;

		void execute();
	};

}
