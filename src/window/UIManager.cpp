//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "UIManager.h"
#include "Window.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glfw/glfw3.h>

namespace tri {

	TRI_SYSTEM(UIManager);

	void UIManager::init() {
		auto* job = env->jobManager->addJob("Render");
		job->addSystem<UIManager>();
		job->orderSystems({ "Window", "UIManager", "Editor", "DebugMenu"});
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

	void UIManager::shutdown() {
		if (ImGui::GetCurrentContext()) {
			ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
			auto& handlers = ImGui::GetCurrentContext()->SettingsHandlers;
			for (int i = 0; i < handlers.size(); i++) {
				auto& handler = handlers[i];
				if (handler.TypeHash == ImHashStr("WindowFlags")) {
					handlers.erase(handlers.begin() + i);
					break;
				}
			}
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
		env->jobManager->getJob("Render")->addSystem(Reflection::getDescriptor(classId)->name);
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
			auto* ui = env->systemManager->getSystem<UIManager>();
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
			auto* ui = env->systemManager->getSystem<UIManager>();
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
