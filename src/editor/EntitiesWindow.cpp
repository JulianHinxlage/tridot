//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "window/Window.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "engine/Random.h"
#include "engine/Time.h"
#include <imgui/imgui.h>

namespace tri {

	class EntitiesWindow : public System {
	public:
		bool& active() {
			return env->systemManager->getSystemHandle(Reflection::getClassId<EntitiesWindow>())->active;
		}

		void init() override {
			env->editor->addWindow<EntitiesWindow>("Entities");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Entities", &active())) {

					if (env->world) {

						if (ImGui::Button("Add Entity")) {
							env->editor->selectedEntity = env->world->addEntity();
						}

						ImGui::Separator();

						if (ImGui::BeginChild("child")) {

							auto* idData = env->world->getEntityStorage()->getIdData();
							int size = env->world->getEntityStorage()->size();
						
							//calculate which entities to show based on window size and scrol position
							float spacing = ImGui::GetStyle().IndentSpacing;
							int start = (ImGui::GetScrollY() - ImGui::GetCursorPos().y) / spacing;
							int end = start + ImGui::GetWindowSize().y * 1.25f / spacing;
							end += 2;
							end = std::min(end, size);
							start = std::max(start, 0);
						
							ImGui::SetCursorPosY(ImGui::GetCursorPos().y + spacing * start);
							for (uint32_t i = start; i < end; i++) {
								EntityId id = idData[i];
								ImGui::PushID(id);

								std::string lable = std::to_string(id);
								if (ImGui::Selectable(lable.c_str(), env->editor->selectedEntity == id)) {
									env->editor->selectedEntity = id;
								}
								if (ImGui::BeginPopupContextItem("context")) {
									if (ImGui::Selectable("remove")) {
										env->world->removeEntity(id);
									}
									ImGui::EndPopup();
								}

								ImGui::PopID();;
							}
							ImGui::SetCursorPosY(ImGui::GetCursorPos().y + spacing * (size - end));


							ImGui::EndChild();
						}

					}

					ImGui::End();
				}
			}
		}
	};
	TRI_SYSTEM(EntitiesWindow);

}