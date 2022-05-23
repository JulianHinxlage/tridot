//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/core.h"
#include "Window.h"
#include "core/Profiler.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace tri {

	class TestUI : public System {
	public:
		virtual void tick() {
			if (env->window->inFrame()) {
				ImGui::DockSpaceOverViewport();
				profiler();
				modules();
			}
		}

		void fullWindow() {
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			if(ImGui::Begin("Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize)) {
				ImGui::End();
			}
			ImGui::PopStyleVar();
		}

		void modules() {
			if (ImGui::Begin("Modules")) {

				if (ImGui::Button("Load Module")) {
					ImGui::OpenPopup("load");
				}
				ImGui::Separator();
				if (ImGui::BeginPopup("load")) {

					for (auto& file : std::filesystem::directory_iterator(".")) {
						if (file.is_regular_file()) {
							if (file.path().extension() == ".dll") {

								if (ImGui::Selectable(file.path().filename().string().c_str())) {
									env->moduleManager->loadModule(file.path().filename().string(), true);
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
					ImGui::PopID();
				}

				ImGui::End();
			}
		}

		void profiler() {
#if TRI_PROFILE_ENABLED
			static int frame = 0;
			if (ImGui::Begin("Profiler")) {
				Profiler::Node *node = env->profiler->getRoot();
				profilerNode(node, frame % 30 == 0);
				ImGui::End();
			}
			frame++;
#endif
		}

		void profilerNode(Profiler::Node* node, bool updateDisplayTime) {
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
			if (node->nodes.size() == 0) {
				flags |= ImGuiTreeNodeFlags_Leaf;
			}

			if (updateDisplayTime) {
				node->displayTime = node->time;
			}

			if (ImGui::TreeNodeEx(node->name, flags)) {
				ImGui::SameLine();
				ImGui::Text("%.3f", node->displayTime * 1000);

				std::vector<Profiler::Node*> nodes;
				for (auto& n : node->nodes) {
					if (n.second) {
						nodes.push_back(n.second.get());
					}
				}
				std::sort(nodes.begin(), nodes.end(), [](Profiler::Node *n1, Profiler::Node *n2) {
					return n1->displayTime > n2->displayTime;
				});

				for (auto* n : nodes) {
						profilerNode(n, updateDisplayTime);
				}
				ImGui::TreePop();
			}
			else {
				ImGui::SameLine();
				ImGui::Text("%.3f", node->displayTime * 1000);
			}
		}
	};
	TRI_SYSTEM(TestUI);

}
