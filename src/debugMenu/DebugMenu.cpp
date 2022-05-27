//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "DebugMenu.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "core/JobManager.h"
#include "core/ModuleManager.h"
#include "core/Profiler.h"
#include "window/Window.h"
#include "core/util/StrUtil.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glfw/glfw3.h>

namespace tri {

	void DebugMenu::init() {
		env->jobManager->addJob("Render")->addSystem<DebugMenu>();
		active = false;
	}

	void DebugMenu::startup() {
		setupFlagsHandler();
	}

	void DebugMenu::tick() {
		bool toggle = false;
		if ((active && (!env->window || !env->window->inFrame()))) {
			toggle = true;
		}
		if (env->editor && !active) {
			toggle = true;
		}
		if (!env->editor && ImGui::IsKeyPressed(GLFW_KEY_F3)) {
			toggle = true;
		}

		if (toggle) {
			active = !active;
			if (!active) {
				for (auto& window : windows) {
					auto* sys = env->systemManager->getSystemHandle(window.classId);
					window.active = sys->active;
					sys->active = false;
				}
			}
			else {
				for (auto& window : windows) {
					auto* sys = env->systemManager->getSystemHandle(window.classId);
					sys->active = window.active;
				}
			}
		}

		if (active && env->window && env->window->inFrame()) {
			if (env->editor) {
				if (ImGui::BeginMainMenuBar()) {
					if (ImGui::BeginMenu("Debug")) {
						for (auto& window : windows) {
							auto* desc = Reflection::getDescriptor(window.classId);
							auto* sys = env->systemManager->getSystemHandle(window.classId);
							if(ImGui::MenuItem(window.displayName.c_str(), nullptr, &sys->active)){}
						}
						ImGui::EndMenu();
					}
					ImGui::EndMainMenuBar();
				}
			}
			else {
				if(ImGui::Begin("Debug Menu")) {
					for (auto &window : windows) {
						auto* desc = Reflection::getDescriptor(window.classId);
						auto* sys = env->systemManager->getSystemHandle(window.classId);
						if (ImGui::Selectable(window.displayName.c_str(), &sys->active)) {}
					}
					ImGui::End();
				}
			}
		}
	}

	void DebugMenu::addWindow(int classId, const std::string& displayName) {
		Window window;
		window.classId = classId;
		window.displayName = displayName;
		window.active = false;
		windows.push_back(window);
		env->jobManager->getJob("Render")->addSystem(Reflection::getDescriptor(classId)->name);
		auto* sys = env->systemManager->getSystemHandle(classId);
		sys->active = false;
	}

	void DebugMenu::setupFlagsHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "OpenDebugFlags";
		handler.TypeHash = ImHashStr("OpenDebugFlags");
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
				for (auto& window : env->systemManager->getSystem<DebugMenu>()->windows) {
					auto* desc = Reflection::getDescriptor(window.classId);
					auto* sys = env->systemManager->getSystemHandle(window.classId);
					if (window.displayName == parts[0]) {
						try {
							sys->active = std::stoi(parts[1]);
							window.active = sys->active;
						}
						catch (...) {}
					}
				}
			}
		};
		handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
			buf->append("[OpenDebugFlags][]\n");
			for (auto& window : env->systemManager->getSystem<DebugMenu>()->windows) {
				auto* desc = Reflection::getDescriptor(window.classId);
				auto* sys = env->systemManager->getSystemHandle(window.classId);
				buf->appendf("%s=%i\n", window.displayName.c_str(), (int)sys->active);
			}
			buf->appendf("\n");
		};
		ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
	}

	TRI_SYSTEM(DebugMenu);


	class DebugWindow : public System {
	public:
		void init() override {
			env->systemManager->getSystem<DebugMenu>()->addWindow<DebugWindow>("Debug");
		}

		bool& active() {
			return env->systemManager->getSystemHandle(Reflection::getClassId<DebugWindow>())->active;
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Debug", &active())) {

					bool vsync = env->window->getVSync();
					if (ImGui::Checkbox("vsync", &vsync)) {
						env->window->setVSync(vsync);
					}

					ImGui::End();
				}
			}
		}
	};
	TRI_SYSTEM(DebugWindow);
}
