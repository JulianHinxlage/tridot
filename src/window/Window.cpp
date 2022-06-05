//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Window.h"
#include "core/core.h"
#include "RenderContext.h"
#include "Viewport.h"
#include "Input.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

	TRI_SYSTEM_INSTANCE(Window, env->window);

	void Window::init() {
		window = nullptr;
		env->jobManager->addJob("Render", { "Window" });
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
		glfwSwapInterval(1);
		vsyncInterval = 1;

		//init imgui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, true);
		const char* glsl_version = "#version 130";
		ImGui_ImplOpenGL3_Init(glsl_version);

		//set imgui style colors
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.3));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.4));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.5));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1, 1, 1, 0.2));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1, 1, 1, 0.3));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1, 1, 1, 0.4));
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1, 1, 1, 0.1));
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1, 1, 1, 0.25));
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(1, 1, 1, 0.40));
		ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(1, 1, 1, 0.1));
		ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(1, 1, 1, 0.25));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.35));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1, 1, 1, 0.45));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1, 1, 1, 0.55));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.35));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1, 1, 1, 0.35));
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 1, 1, 0.45));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.116, 0.125, 0.133, 1));
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.177, 0.177, 0.177, 1));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.238, 0.238, 0.238, 1));
		ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(0.0, 0.32, 1.0, 1));

		ImGui::GetIO().IniFilename = "layout.ini";
	}

	void Window::updateBegin() {
		TRI_PROFILE_FUNC();
		if (window) {
			glfwPollEvents();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			int x = 0;
			int y = 0;
			glfwGetFramebufferSize((GLFWwindow*)window, &x, &y);
			glViewport(0, 0, x, y);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			inFrameFlag = true;
		}
	}

	void Window::updateEnd() {
		TRI_PROFILE_FUNC();
		if (window && inFrameFlag) {
			env->input->allowInputs = !ImGui::GetIO().WantTextInput;
			{
				TracyGpuZone("imgui render");
				ImGui::Render();
			}

			{
				TracyGpuZone("imgui render draw data");
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			{
				TracyGpuZone("swap buffers");
				glfwSwapBuffers((GLFWwindow*)window);
			}
			TracyGpuCollect;

			if (!isOpen()) {
				env->console->setCVarValue("running", false);
			}
			ImGui::GetIO().MouseWheel += env->input->getMouseWheelDelta();
		}
		inFrameFlag = false;
	}

	void Window::tick() {
		TRI_PROFILE_FUNC();
		if (env->viewport->displayInWindow) {
			int width = 0;
			int height = 0;
			glfwGetWindowSize((GLFWwindow*)window, &width, &height);
			env->viewport->size = { width, height };
		}
		updateEnd();
		updateBegin();
	}

	void Window::shutdown() {
		TRI_PROFILE_FUNC();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow((GLFWwindow*)window);
		glfwTerminate();
		inFrameFlag = false;
	}

	void Window::setVSync(int interval) {
		if (vsyncInterval != interval) {
			glfwSwapInterval(interval);
		}
		vsyncInterval = interval;
	}

	int Window::getVSync() {
		return vsyncInterval;
	}

	void Window::close() {
		glfwSetWindowShouldClose((GLFWwindow*)window, 1);
	}

	bool Window::isOpen() {
		return glfwWindowShouldClose((GLFWwindow*)window) == 0;
	}

	bool Window::inFrame() {
		if (!ImGui::GetCurrentContext()) {
			return false;
		}
		return inFrameFlag;
	}

	void* Window::getContext() {
		return window;
	}

}