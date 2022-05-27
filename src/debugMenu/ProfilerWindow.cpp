//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "DebugMenu.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "core/Profiler.h"
#include "window/Window.h"

#include <imgui/imgui.h>

namespace tri {

	class ProfilerWindow : public System {
	public:
		void init() override {
#if TRI_PROFILE_ENABLED
			env->systemManager->getSystem<DebugMenu>()->addWindow<ProfilerWindow>("Profiler");
#endif
		}

		bool& active() {
			return env->systemManager->getSystemHandle(Reflection::getClassId<ProfilerWindow>())->active;
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
#if TRI_PROFILE_ENABLED
				static int frame = 0;
				if (ImGui::Begin("Profiler", &active())) {
					Profiler::Node* node = env->profiler->getRoot();
					profilerNode(node, frame % 30 == 0);
					ImGui::End();
				}
				frame++;
#endif
			}
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
				std::sort(nodes.begin(), nodes.end(), [](Profiler::Node* n1, Profiler::Node* n2) {
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

	TRI_SYSTEM(ProfilerWindow);

}