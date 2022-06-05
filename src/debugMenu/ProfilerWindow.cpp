//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "window/UIManager.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "core/Profiler.h"
#include "window/Window.h"
#include "engine/Time.h"

#include <imgui/imgui.h>

namespace tri {

	class ProfilerWindow : public UIWindow {
	public:
		float updateInterval = 1.0f;

		float fps = 0;
		float avg = 0;
		float min = 0;
		float max = 0;

		void init() override {
			env->systemManager->getSystem<UIManager>()->addWindow<ProfilerWindow>("Profiler", "Debug");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Profiler", &active)) {
					bool updateDisplayTime = env->time->frameTicks(updateInterval);
					if (updateDisplayTime) {
						fps = env->time->framesPerSecond;
						avg = env->time->avgFrameTime * 1000.0f;
						min = env->time->minFrameTime * 1000.0f;
						max = env->time->maxFrameTime * 1000.0f;
					}
					ImGui::Text("FPS: %f", fps);
					ImGui::Text("Avg: %f ms", avg);
					ImGui::Text("Max: %f ms", max);
					ImGui::Text("Min: %f ms", min);
#if TRI_PROFILE_ENABLED
					ImGui::Separator();
					Profiler::Node* node = env->profiler->getRoot();
					profilerNode(node, updateDisplayTime);
#endif
				}
				ImGui::End();
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