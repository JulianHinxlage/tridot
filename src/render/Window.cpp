//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Window.h"
#include "core/core.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

namespace tri {

	TRI_SYSTEM_INSTANCE(Window, env->window);

	void Window::init() {
		auto *job = env->jobManager->addJob("Render", { "Window", "TestUI" });
	}

	void Window::startup() {
		TRI_PROFILE_FUNC();
		int width = env->console->getCVarValue<int>("windowWidth", 800);
		int height = env->console->getCVarValue<int>("windowHeight", 600);
		std::string title = env->console->getCVarValue<std::string>("windowTitle", "Tridot Engine");

		//init glfw
		glfwSetErrorCallback(glfw_error_callback);

		if (!glfwInit()) {
			printf("failed to initialize GLFW\n");
			return;
		}

		//create window
		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		window = (void*)glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!window) {
			printf("failed to create Window\n");
			return;
		}
		glfwMakeContextCurrent((GLFWwindow*)window);
		glfwSwapInterval(1);

		//init glew
		if (glewInit() != GLEW_OK) {
			printf("failed to initialize OpenGL\n");
			return;
		}


		//init imgui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		inFrameFlag = true;
	}

	void Window::updateEnd() {
		TRI_PROFILE_FUNC();
		if (inFrameFlag) {
			ImGui::Render();
			int x = 0;
			int y = 0;
			glfwGetFramebufferSize((GLFWwindow*)window, &x, &y);
			glViewport(0, 0, x, y);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			glfwSwapBuffers((GLFWwindow*)window);

			if (!isOpen()) {
				env->console->setCVarValue("running", false);
			}
		}
		inFrameFlag = false;
	}

	void Window::tick() {
		TRI_PROFILE_FUNC();
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
		env->console->info("Window::shutdown()");
	}

	void Window::close() {
		glfwSetWindowShouldClose((GLFWwindow*)window, 1);
	}

	bool Window::isOpen() {
		return glfwWindowShouldClose((GLFWwindow*)window) == 0;
	}

	bool Window::inFrame() {
		return inFrameFlag;
	}

}