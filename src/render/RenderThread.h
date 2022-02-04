//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

	class RenderThread : public System {
	public:
		bool runOnMainThread;

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
		std::mutex dataMutex;
		std::condition_variable cv;
		std::atomic_bool running;

		std::vector<std::function<void()>> tasks;

		void execute();
	};

}
