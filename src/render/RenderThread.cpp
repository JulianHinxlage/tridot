//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderThread.h"
#include "RenderPipeline.h"
#include "RenderContext.h"
#include "Window.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace tri {

	TRI_REGISTER_CALLBACK() {
		env->renderThread = new RenderThread();
		env->systems->setSystem("RenderThread", env->renderThread);
		env->signals->update.callbackOrder({ "Editor", "RenderThread" });
	}

	RenderThread::RenderThread() {
		thread = nullptr;
		running = false;
		runOnMainThread = false;
	}

	void RenderThread::update() {
		synchronize();
	}

	void RenderThread::terminate() {
		running = false;
		if (thread != nullptr) {
			cv.notify_all();
			thread->join();
			delete thread;
		}
		thread = nullptr;
	}

	void RenderThread::synchronize() {
		if (!runOnMainThread) {
			std::unique_lock<std::mutex> lock(mutex);
			cv.notify_one();
			cv.wait(lock);
			cv.notify_one();
		}
		else {
			execute();
		}
	}

	void RenderThread::lock() {
		dataMutex.lock();
	}

	void RenderThread::unlock() {
		dataMutex.unlock();
	}

	void RenderThread::addTask(const std::function<void()>& task) {
		tasks.push_back(task);
	}

	void RenderThread::launch(const std::function<void()>& init) {
		if (!running) {
			running = true;
			if (!runOnMainThread) {
				thread = new std::thread([&, init]() {
					if (init) {
						init();
					}
					while (running) {
						std::unique_lock<std::mutex> lock(mutex);
						cv.notify_one();
						cv.wait(lock);
						cv.notify_one();

						execute();
					}
				});
			}
			else {
				if (init) {
					init();
				}
			}
		}
	}

	void RenderThread::execute() {
		for (auto& task : tasks) {
			if (task) {
				task();
			}
		}
		tasks.clear();

		env->pipeline->execute();
	}

}
