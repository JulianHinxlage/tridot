//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Window.h"
#include "core/core.h"
#include "RenderContext.h"
#include "Viewport.h"
#include "Input.h"
#include "UIManager.h"
#include <imgui.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tracy/TracyOpenGL.hpp>

#if TRI_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#undef max
#endif

namespace tri {

	TRI_SYSTEM_INSTANCE(Window, env->window);

	void Window::init() {
		window = nullptr;
		env->jobManager->addJob("Render", { "Window" });
		vsyncInterval = (int)env->console->addCVar<bool>("vsync", true)->get<bool>();
	}

	void Window::startup() {
		TRI_PROFILE_FUNC();
		int width = env->console->getCVarValue<int>("windowWidth", 800);
		int height = env->console->getCVarValue<int>("windowHeight", 600);
		std::string title = env->console->getCVarValue<std::string>("windowTitle", "Tridot Engine");
		if (env->console->getCVarValue<bool>("noWindow", false)) {
			return;
		}

		window = RenderContext::create();
		glfwSetWindowSize((GLFWwindow*)window, width, height);
		glfwSetWindowTitle((GLFWwindow*)window, title.c_str());
		glfwSwapInterval(vsyncInterval);

		//set window position by monitor
		int monitorCount = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
		GLFWmonitor* monitor = nullptr;
		int monitorIndex = env->console->getCVarValue<int>("monitor", -1);
		if (monitorIndex >= 0 && monitorIndex < monitorCount) {
			monitor = monitors[monitorIndex];
		}
		if (monitor) {
			int x = 0;
			int y = 0;
			int w = 0;
			int h = 0;
			glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
			glfwSetWindowPos((GLFWwindow*)window, x + w / 2 - width / 2, std::max(y + h / 2 - height / 2 + 30, 30));
		}
	}

	void Window::frameBegin() {
		TRI_PROFILE_FUNC();
		if (window) {
			glfwPollEvents();
			if (env->uiManager) {
				env->uiManager->frameBegin();
			}

			int vsync = env->console->getCVarValue<bool>("vsync", true);
			if (vsync != vsyncInterval) {
				setVSync(vsync);
			}

			int x = 0;
			int y = 0;
			glfwGetFramebufferSize((GLFWwindow*)window, &x, &y);
			glViewport(0, 0, x, y);
			glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			inFrameFlag = true;
		}
	}

	void Window::frameEnd() {
		TRI_PROFILE_FUNC();
		if (window && inFrameFlag) {
			if (env->uiManager) {
				env->uiManager->frameEnd();
			}

			{
				TRI_PROFILE("swap buffers");
				TracyGpuZone("swap buffers");
				glfwSwapBuffers((GLFWwindow*)window);
			}
			TracyGpuCollect;

			if (!isOpen()) {
				env->console->setCVarValue("running", false);
			}
		}
		inFrameFlag = false;
	}

	void Window::tick() {
		if (env->viewport->displayInWindow) {
			int width = 0;
			int height = 0;
			glfwGetWindowSize((GLFWwindow*)window, &width, &height);
			env->viewport->size = { width, height };
		}
		frameEnd();
		frameBegin();
	}

	void Window::shutdown() {
		TRI_PROFILE_FUNC();
		if (env->uiManager) {
			env->uiManager->frameShutdown();
		}
		
		glfwDestroyWindow((GLFWwindow*)window);
		glfwTerminate();
		inFrameFlag = false;
	}

	void Window::setVSync(int interval) {
		if (vsyncInterval != interval) {
			glfwSwapInterval(interval);
			env->console->setCVarValue<bool>("vsync", (bool)interval);
		}
		vsyncInterval = interval;
	}

	int Window::getVSync() {
		return vsyncInterval;
	}

	void Window::setBackgroundColor(const glm::vec4& color) {
		backgroundColor = color;
	}

	glm::vec2 Window::getPosition() {
		int x, y;
		glfwGetWindowPos((GLFWwindow*)window, &x, &y);
		return glm::vec2(x, y);
	}

	void Window::close() {
		glfwSetWindowShouldClose((GLFWwindow*)window, 1);
	}

	bool Window::isOpen() {
		return glfwWindowShouldClose((GLFWwindow*)window) == 0;
	}

	bool Window::inFrame() {
		if (env->uiManager) {
			if (!ImGui::GetCurrentContext()) {
				return false;
			}
		}
		return inFrameFlag;
	}

	void* Window::getContext() {
		return window;
	}

	void* Window::getNativeContext() {
#if TRI_WINDOWS
		return glfwGetWin32Window((GLFWwindow*)window);
#else
		return nullptr;
#endif
	}

}
