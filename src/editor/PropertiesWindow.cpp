//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "entity/World.h"
#include "window/Window.h"

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace tri {

	class PropertiesWindow : public System {
	public:
		bool& active() {
			return env->systemManager->getSystemHandle(Reflection::getClassId<PropertiesWindow>())->active;
		}

		void init() override {
			env->editor->addWindow<PropertiesWindow>("Properties");
			//active() = true;
		}

		void classUi(int classId, void *ptr) {
			auto* desc = Reflection::getDescriptor(classId);

			for (auto& prop : desc->properties) {
				if (!(prop.flags & PropertyDescriptor::HIDDEN)) {
					void* propPtr = (uint8_t*)ptr + prop.offset;
					if (prop.type->isType<int>()) {
						ImGui::DragInt(prop.name.c_str(), (int*)propPtr);
					}
					else if (prop.type->isType<float>()) {

						if (prop.min && prop.max) {
							ImGui::SliderFloat(prop.name.c_str(), (float*)propPtr, *(float*)prop.min, *(float*)prop.max);
						}
						else {
							ImGui::DragFloat(prop.name.c_str(), (float*)propPtr, 0.1);
						}

					}
					else if (prop.type->isType<double>()) {
						ImGui::InputDouble(prop.name.c_str(), (double*)propPtr, 0.1);
					}
					else if (prop.type->isType<bool>()) {
						ImGui::Checkbox(prop.name.c_str(), (bool*)propPtr);
					}
					else if (prop.type->isType<std::string>()) {
						ImGui::InputText(prop.name.c_str(), (std::string*)propPtr);
					}
					else if (prop.type->isType<glm::vec2>()) {
						ImGui::DragFloat2(prop.name.c_str(), (float*)propPtr, 0.1);
					}
					else if (prop.type->isType<glm::vec3>()) {
						ImGui::DragFloat3(prop.name.c_str(), (float*)propPtr, 0.1);
					}
					else if (prop.type->isType<glm::vec4>()) {
						ImGui::DragFloat4(prop.name.c_str(), (float*)propPtr, 0.1);
					}
					else {
						if (ImGui::TreeNode(prop.name.c_str())) {
							classUi(prop.type->classId, propPtr);
							ImGui::TreePop();
						}
					}
				}
			}
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Properties", &active())) {

					if (env->world) {
						EntityId id = env->editor->selectedEntity;
						if (env->editor->selectedEntity != -1) {

							if (ImGui::Button("Add Component")) {
								ImGui::OpenPopup("add");
							}

							if (ImGui::BeginPopup("add")) {
								for (auto* desc : Reflection::getDescriptors()) {
									if (desc && desc->flags & ClassDescriptor::COMPONENT) {
										if (!env->world->hasComponent(id, desc->classId)) {
											if (ImGui::Selectable(desc->name.c_str())) {
												env->world->addComponent(id, desc->classId);
											}
										}
									}
								}
								ImGui::EndPopup();
							}

							ImGui::Separator();

							if (ImGui::BeginChild("child")) {
								for (auto* desc : Reflection::getDescriptors()) {
									if (desc && desc->flags & ClassDescriptor::COMPONENT) {
										if (env->world->hasComponent(id, desc->classId)) {
											ImGui::PushID(desc->classId);
											if (ImGui::Button("-")) {
												env->world->removeComponent(id, desc->classId);
											}
											else {
												ImGui::SameLine();
												if (ImGui::CollapsingHeader(desc->name.c_str())) {
													classUi(desc->classId, env->world->getComponent(id, desc->classId));
												}
											}
											ImGui::PopID();
										}
									}
								}

								ImGui::EndChild();
							}
						}

					}

					ImGui::End();
				}
			}
		}
	};
	TRI_SYSTEM(PropertiesWindow);

}