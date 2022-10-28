//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "window/UIManager.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "core/ModuleManager.h"
#include "window/Window.h"

#include <imgui/imgui.h>

namespace tri {

	class ModulesWindow : public UIWindow {
	public:
		void init() override {
			env->uiManager->addWindow<ModulesWindow>("Modules", "Debug");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Modules", &active)) {

					if (ImGui::Button("Load Module")) {
						ImGui::OpenPopup("load");
					}
					ImGui::Separator();
					if (ImGui::BeginPopup("load")) {

						for (auto& dir : env->moduleManager->getModuleDirectories()) {
							for (auto& file : std::filesystem::directory_iterator(dir)) {
								if (file.is_regular_file()) {
									if (file.path().extension() == ".dll") {

										if (ImGui::Selectable(file.path().filename().string().c_str())) {
											env->moduleManager->loadModule(file.path().filename().string(), true);
										}
									}
								}
							}
						}

						ImGui::EndPopup();
					}

					for (auto& m : env->moduleManager->getModules()) {
						ImGui::PushID(m->name.c_str());
						if (ImGui::Button("Unload")) {
							env->moduleManager->unloadModule(m.get(), true);
						}
						ImGui::SameLine();
						if (ImGui::Button("Reload")) {
							env->moduleManager->unloadModule(m.get(), true);
							env->moduleManager->loadModule(m->name, true);
						}
						ImGui::SameLine();
						ImGui::Text("%s", m->name.c_str());

						std::string autoLoaded;
						for (auto& d : m->autoLoaded) {
							if (d) {
								if (!autoLoaded.empty()) {
									autoLoaded += ", ";
								}
								autoLoaded += d->name;
							}
						}
						if (!autoLoaded.empty()) {
							ImGui::Text("auto loaded: %s", autoLoaded.c_str());
						}

						ImGui::PopID();
					}

				}
				ImGui::End();
			}
		}
	};

	TRI_SYSTEM(ModulesWindow);

}