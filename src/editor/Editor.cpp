//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "core/JobManager.h"
#include "core/config.h"
#include "window/Window.h"
#include <imgui.h>
#include <imgui/imgui_internal.h>

namespace tri {

	TRI_SYSTEM_INSTANCE(Editor, env->editor);

	void Editor::init() {
		auto *job = env->jobManager->addJob("Render");
		job->addSystem<Editor>();
		job->orderSystems({ "Editor", "DebugMenu" });
		menus = { "View", "Debug" };
	}

	void Editor::startup() {
		setupFlagsHandler();
	}

	void Editor::tick() {
		if (env->window && env->window->inFrame()) {
			ImGui::DockSpaceOverViewport();

			bool openAbout = false;
			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					ImGui::EndMenu();
				}

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
							if (ImGui::MenuItem(window.displayName.c_str(), nullptr, &sys->active)) {}
							if (!window.category.empty()) {
								ImGui::EndMenu();
							}
						}

						ImGui::EndMenu();
					}

				}


				if (ImGui::BeginMenu("Extra")) {
					if (ImGui::MenuItem("About")) {
						openAbout = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (openAbout) {
				ImGui::OpenPopup("About");
			}

			bool open = true;
			if (ImGui::BeginPopupModal("About", &open)) {
				ImGui::Text("Tridot Engine");
				ImGui::Text("Version %s", TRI_VERSION);
				ImGui::Text("By Julian Hinxlage");
				if (!open) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
	}

	void Editor::addWindow(int classId, const std::string& displayName, const std::string& menu, const std::string& category) {
		Window window;
		window.classId = classId;
		window.displayName = displayName;
		window.menu = menu;
		window.category = category;
		windows.push_back(window);
		env->jobManager->getJob("Render")->addSystem(Reflection::getDescriptor(classId)->name);
		auto* sys = env->systemManager->getSystemHandle(classId);
		sys->active = false;
	}

	void Editor::setupFlagsHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "OpenFlags";
		handler.TypeHash = ImHashStr("OpenFlags");
		handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
			if (std::string(name) == "") {
				return (void*)1;
			}
			else {
				return nullptr;
			}
		};
		handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
			auto parts = StrUtil::split(line, "=");
			if (parts.size() >= 2) {
				for (auto& window : env->editor->windows) {
					auto* desc = Reflection::getDescriptor(window.classId);
					auto* sys = env->systemManager->getSystemHandle(window.classId);
					if (window.displayName == parts[0]) {
						try {
							sys->active= std::stoi(parts[1]);
						}
						catch (...) {}
					}
				}
			}
		};
		handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
			buf->append("[OpenFlags][]\n");
			for (auto& window : env->editor->windows) {
				auto* desc = Reflection::getDescriptor(window.classId);
				auto* sys = env->systemManager->getSystemHandle(window.classId);
				buf->appendf("%s=%i\n", window.displayName.c_str(), (int)sys->active);
			}
			buf->appendf("\n");
		};
		ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
	}

}
