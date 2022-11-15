//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "UIManager.h"
#include "Window.h"
#include "Input.h"
#include <imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

	TRI_SYSTEM_INSTANCE(UIManager, env->uiManager);

	void UIManager::init() {
		auto* job = env->jobManager->addJob("Editor");
		job->addSystem<UIManager>();
		job->orderSystems({ "Window", "UIManager", "Editor"});
		if (!uiOnOwnThread) {
			env->jobManager->addJob("Render")->addChildJob("Editor");
		}
		menus = { "File", "View", "Debug" };

		env->eventManager->onClassUnregister.addListener([&](int classId) {
			for (int i = 0; i < windows.size(); i++) {
				auto& window = windows[i];
				if (window.classId == classId) {
					unusedActiveFlags.push_back({ window.displayName, window.active });
					windows.erase(windows.begin() + i);
					break;
				}
			}
		});
	}
	
	void UIManager::startup() {
		//init imgui
		IMGUI_CHECKVERSION();
		imguiContext = (void*)ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		if (!uiOnOwnThread) {
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		}
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}


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



		layoutFiles = { "layout.ini" , "../layout.ini" , "../../layout.ini" };
		if (!env->editor) {
			layoutFiles = { "layout2.ini" , "../layout2.ini" , "../../layout2.ini" };
		}
		if (!std::filesystem::exists(layoutFiles[0])) {
			for (int i = 1; i < layoutFiles.size(); i++) {
				if (std::filesystem::exists(layoutFiles[i])) {
					std::filesystem::copy(layoutFiles[i], layoutFiles[0]);
					break;
				}
			}
		}
		ImGui::GetIO().IniFilename = layoutFiles[0];


		setupFlagHandler();
		if (ImGui::GetCurrentContext()) {
			ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);
		}
		if (env->editor) {
			active = true;
		}
		else {
			active = false;
		}
	}

	void UIManager::tick() {
		if (!env->editor) {
			bool toggle = false;
			if (ImGui::IsKeyPressed(GLFW_KEY_F3)) {
				toggle = true;
			}

			if (toggle) {
				active = !active;
			}
		}

		updateActiveFlags();

		if (!initialized) {
			return;
		}

		if (uiOnOwnThread) {
			renderDrawData();
		}

		if (active) {
			if (env->editor) {
				if (env->window && env->window->inFrame()) {
					ImGui::DockSpaceOverViewport();

					if (ImGui::BeginMainMenuBar()) {
						for (auto& menu : menus) {
							if (ImGui::BeginMenu(menu.c_str())) {
								ImGui::EndMenu();
							}
						}

						for (auto& window : windows) {
							if (ImGui::BeginMenu(window.menu.c_str())) {
								auto* desc = Reflection::getDescriptor(window.classId);
								auto* sys = env->systemManager->getSystemHandle(window.classId);

								if (window.category.empty() || ImGui::BeginMenu(window.category.c_str())) {
									if (ImGui::MenuItem(window.displayName.c_str(), nullptr, &window.window->active)) {}
									if (!window.category.empty()) {
										ImGui::EndMenu();
									}
								}

								ImGui::EndMenu();
							}
						}

						ImGui::EndMainMenuBar();
					}
				}

			}
			else {
				if (windows.size() > 0) {
					if (ImGui::Begin("Debug Menu")) {
						for (auto& window : windows) {
							auto* desc = Reflection::getDescriptor(window.classId);
							auto* sys = env->systemManager->getSystemHandle(window.classId);
							if (ImGui::Selectable(window.displayName.c_str(), &window.window->active)) {}
						}
					}
					ImGui::End();
				}
			}
		}
	}

	void UIManager::renderDrawData() {
		{
			TRI_PROFILE("wait");
			ZoneColor(tracy::Color::DimGray);
			while (drawDataReady) {}
		}

		std::unique_lock<std::mutex> lock(mutex);

		if (ImGui::GetCurrentWindowRead()) {
			TRI_PROFILE("ImGui::Render");
			ImGui::Render();
		}

		if (ImGui::GetDrawData()) {
			auto* data = ImGui::GetDrawData();
			if (uiOnOwnThread) {
				ImDrawData* newData = new ImDrawData(*data);
				drawData = newData;
			}
			else {
				drawData = data;
			}
			drawDataReady = true;
		}

		if (uiOnOwnThread) {
			{
				{
					TRI_PROFILE("wait");
					ZoneColor(tracy::Color::DimGray);
					while (!drawDataFinished) {}
				}

				TRI_PROFILE("ImGui::NewFrame");
				ImGui::NewFrame();

				drawDataFinished = false;
			}
		}
	}

	void UIManager::frameBegin() {
		if (!initialized) {
			initialized = true;
			ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)env->window->getContext(), true);
			const char* glsl_version = "#version 130";
			ImGui_ImplOpenGL3_Init(glsl_version);
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		drawDataFinished = true;

		if (!uiOnOwnThread) {
			{
				TRI_PROFILE("ImGui::NewFrame");
				ImGui::NewFrame();
			}
		}
	}

	void UIManager::frameEnd() {
		if (!uiOnOwnThread) {
			renderDrawData();
		}

		std::unique_lock<std::mutex> lock(mutex);

		env->input->allowInputs = !ImGui::GetIO().WantTextInput;


		{
			TRI_PROFILE("ImGuiRenderDrawData");
			TracyGpuZone("ImGuiRenderDrawData");
			if (drawData) {
				ImGui_ImplOpenGL3_RenderDrawData((ImDrawData*)drawData);
				if (uiOnOwnThread) {
					delete (ImDrawData*)drawData;
				}
				drawData = nullptr;
				drawDataFinished = false;
				drawDataReady = false;
			}
		}

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		ImGui::GetIO().MouseWheel += env->input->getMouseWheelDelta();
	}

	void UIManager::shutdown() {
		std::unique_lock<std::mutex> lock(mutex);
		if (ImGui::GetCurrentContext()) {
			ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
		}
		ImGui::DestroyContext();
	}

	void UIManager::frameShutdown() {
		std::unique_lock<std::mutex> lock(mutex);
		if (ImGui::GetCurrentContext()) {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		}
	}

	void UIManager::addWindow(int classId, const std::string& displayName, const std::string& menu, const std::string& category) {
		Window window;
		window.classId = classId;
		window.displayName = displayName;
		window.menu = menu;
		window.category = category;
		window.window = (UIWindow*)env->systemManager->getSystem(classId);
		if (window.window) {
			window.window->active = false;
			for (int i = 0; i < unusedActiveFlags.size(); i++) {
				if (unusedActiveFlags[i].first == displayName) {
					window.window->active = unusedActiveFlags[i].second;
					unusedActiveFlags.erase(unusedActiveFlags.begin() + i);
					break;
				}
			}
		}
		windows.push_back(window);
		auto* job = env->jobManager->addJob("Editor");
		job->addSystem(Reflection::getDescriptor(classId)->name);
		auto* sys = env->systemManager->getSystemHandle(classId);
		sys->active = false;
	}

	void UIManager::updateActiveFlags() {
		if (active) {
			for (auto& window : windows) {
				auto* handle = env->systemManager->getSystemHandle(window.classId);
				handle->active = window.window->active;
				window.active = window.window->active;
			}
		}
		else {
			for (auto& window : windows) {
				auto* handle = env->systemManager->getSystemHandle(window.classId);
				handle->active = false;
			}
		}
	}

	void UIManager::resetLayout() {
		env->eventManager->postTick.addListener([&]() {
			for (auto& window : windows) {
				auto* sys = env->systemManager->getSystemHandle(window.classId);
				window.window->active = false;
				sys->active = false;
			}
			for (auto& flag : unusedActiveFlags) {
				flag.second = false;
			}

			for (int i = 1; i < layoutFiles.size(); i++) {
				if (std::filesystem::exists(layoutFiles[i])) {
					ImGui::LoadIniSettingsFromDisk(layoutFiles[i]);
					break;
				}
			}

			updateActiveFlags();
		}, true);
	}

	void UIManager::setupFlagHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "WindowFlags";
		handler.TypeHash = ImHashStr("WindowFlags");
		handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
			if (std::string(name) == "") {
				return (void*)1;
			}
			else {
				return nullptr;
			}
		};
		handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
			auto* ui = env->uiManager;
			auto parts = StrUtil::split(line, "=");
			if (parts.size() >= 2) {
				bool found = false;
				for (auto& window : ui->windows) {
					auto* desc = Reflection::getDescriptor(window.classId);
					auto* sys = env->systemManager->getSystemHandle(window.classId);
					if (window.displayName == parts[0]) {
						found = true;
						try {
							window.window->active = std::stoi(parts[1]);
							sys->active = window.window->active;
						}
						catch (...) {}
					}
				}
				if (!found) {
					try {
						ui->unusedActiveFlags.push_back({ parts[0], std::stoi(parts[1]) });
					}
					catch (...) {}
				}
			}
		};
		handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
			auto* ui = env->uiManager;
			buf->append("[WindowFlags][]\n");
			for (auto& window : ui->windows) {
				auto* desc = Reflection::getDescriptor(window.classId);
				auto* sys = env->systemManager->getSystemHandle(window.classId);
				buf->appendf("%s=%i\n", window.displayName.c_str(), (int)window.window->active);
			}
			for (auto& flag : ui->unusedActiveFlags) {
				buf->appendf("%s=%i\n", flag.first.c_str(), (int)flag.second);
			}

			buf->appendf("\n");
		};
		if (ImGui::GetCurrentContext()) {
			ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
		}
	}

}
