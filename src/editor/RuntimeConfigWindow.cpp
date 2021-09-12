//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include <imgui/imgui.h>

namespace tri {

	class RuntimeConfigWindow : public EditorWindow {
	public:
		std::set<std::string> allwaysOn;
		bool doNotChangeState = false;

		void startup() {
			name = "Runtime Config";
			isDebugWindow = true;
			allwaysOn = {
				"Editor",
				"Window",
				"Imgui.begin",
				"Imgui.end",
			};
		}

		void update() override {
			auto& observers = env->signals->update.getObservers();
			for (int i = 0; i < observers.size(); i++) {
				auto& observer = observers[i];

				bool active = observer.active;
				bool canChange = !allwaysOn.contains(observer.name);
				if (!canChange) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				}
				if (ImGui::Selectable(observer.name.c_str(), &active)) {
					if (!doNotChangeState && canChange) {
						env->signals->update.setActiveCallback(observer.name, active);
					}
					doNotChangeState = false;
				}
				if (!canChange) {
					ImGui::PopStyleColor();
				}

				if (canChange && ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
					int swapIndex = i + (ImGui::GetMouseDragDelta(0).y < 0.0f ? -1.0f : 1.0f);
					if (swapIndex >= 0 && swapIndex < observers.size()) {
						env->signals->update.swapObserverPositions(i, swapIndex);
						ImGui::ResetMouseDragDelta();
						doNotChangeState = true;
					}
				}
			}
		}

	};
	TRI_STARTUP_CALLBACK("") {
		editor->addWindow(new RuntimeConfigWindow);
	}

}
