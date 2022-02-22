//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderThread.h"
#include "RenderPipeline.h"
#include "RenderContext.h"
#include "Window.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "tracy/TracyOpenGL.hpp"

namespace tri {

	TRI_REGISTER_CALLBACK() {
		env->renderThread = new RenderThread();
		env->systems->setSystem("RenderThread", env->renderThread);
		env->signals->update.callbackOrder({ "Editor", "RenderThread" });
	}

	RenderThread::RenderThread() : barrier(2) {
		thread = nullptr;
		running = false;
		useDedicatedThread = false;
		executeRenderPipeline = true;
	}

	void RenderThread::update() {
		synchronize();
	}

	void RenderThread::terminate() {
		running = false;
		if (thread != nullptr) {
			barrier.arrive();
			thread->join();
			delete thread;
		}
		thread = nullptr;
	}

	void RenderThread::synchronize() {
		if (useDedicatedThread) {
			ZoneScopedNC("waitForRenderThread", tracy::Color::Gray30);
			barrier.arrive_and_wait();
			currentTasks.swap(tasks);
			tasks.clear();
			barrier.arrive_and_wait();
		}
		else {
			currentTasks.swap(tasks);
			tasks.clear();
			env->renderPipeline->prepareRenderPasses();
			execute();
		}
	}

	void RenderThread::lock() {
		mutex.lock();
	}

	void RenderThread::unlock() {
		mutex.unlock();
	}

	void RenderThread::addTask(const std::function<void()>& task) {
		tasks.push_back(task);
	}

	void RenderThread::launch(const std::function<void()>& init) {
		if (!running) {
			running = true;
			if (useDedicatedThread) {
				thread = new std::thread([&, init]() {
					TRI_PROFILE_THREAD("Render Thread");
					if (init) {
						init();
					}
					barrier.arrive_and_wait();
					while (running) {
						{
							ZoneScopedNC("waitForMainThread", tracy::Color::Gray30);
							barrier.arrive_and_wait();
						}
						env->renderPipeline->prepareRenderPasses();
						barrier.arrive_and_wait();

						TRI_PROFILE("RenderThread");
						execute();
					}
				});
				barrier.arrive_and_wait();
			}
			else {
				if (init) {
					init();
				}
			}
		}
	}

	void RenderThread::execute() {
		for (auto& task : currentTasks) {
			if (task) {
				TRI_PROFILE("Task");
				TracyGpuZone("Task");
				task();
			}
		}

		if (executeRenderPipeline) {
			env->renderPipeline->execute();
		}
	}

}
