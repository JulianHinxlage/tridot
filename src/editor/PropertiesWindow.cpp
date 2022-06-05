//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "window/UIManager.h"
#include "ClassUI.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "entity/World.h"
#include "window/Window.h"
#include "engine/EntityInfo.h"

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace tri {

	class PropertiesWindow : public UIWindow {
	public:
		int currentChangeClassId = -1;
		DynamicObjectBuffer singleEditPreChangeValue;
		std::vector<std::pair<EntityId, DynamicObjectBuffer>> multiEditPreChangeValues;

		void init() override {
			env->systemManager->getSystem<UIManager>()->addWindow<PropertiesWindow>("Properties");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Properties", &active)) {
					if (env->world) {

						if (env->editor->selectionContext->isSingleSelection()) {
							EntityId id = env->editor->selectionContext->getSelected()[0];
							if (env->world->hasEntity(id)) {
								header();
								if (ImGui::BeginChild("child")) {
									singleEdit(id);
									contextMenu();
									ImGui::EndChild();
								}
							}
						}
						else if (env->editor->selectionContext->isMultiSelection()) {
							header();
							if (ImGui::BeginChild("child")) {
								multiEdit();
								contextMenu();
								ImGui::EndChild();
							}
						}
						else {
							currentChangeClassId = -1;
						}

					}
				}
				ImGui::End();
			}
		}

		void header() {
			if (ImGui::Button("Add Component")) {
				ImGui::OpenPopup("add");
			}

			if (ImGui::BeginPopup("add")) {
				for (auto* desc : Reflection::getDescriptors()) {
					if (desc && desc->flags & ClassDescriptor::COMPONENT && !(desc->flags & ClassDescriptor::HIDDEN)) {

						bool haveAll = true;
						for (auto& id : env->editor->selectionContext->getSelected()) {
							if (!env->world->hasComponent(id, desc->classId)) {
								haveAll = false;
							}
						}

						if (!haveAll) {
							if (desc->category.empty() || ImGui::BeginMenu(desc->category.c_str())) {
								if (ImGui::MenuItem(desc->name.c_str())) {

									env->editor->undo->beginAction();
									for (auto& id : env->editor->selectionContext->getSelected()) {
										if (!env->world->hasComponent(id, desc->classId)) {
											env->world->addComponent(id, desc->classId);
											env->editor->undo->componentAdded(desc->classId, id);
										}
									}
									env->editor->undo->endAction();

								}
								if (!desc->category.empty()) {
									ImGui::EndMenu();
								}
							}
						}

					}
				}
				ImGui::EndPopup();
			}

			if (env->editor->selectionContext->isSingleSelection()) {
				EntityId id = env->editor->selectionContext->getSelected()[0];
				if (auto* info = env->world->getComponent<EntityInfo>(id)) {
					singleEditComponent(id, Reflection::getClassId<EntityInfo>(), info, true);
				}
			}
			else if (env->editor->selectionContext->isMultiSelection()) {
				auto* desc = Reflection::getDescriptor<EntityInfo>();
				void* comp = nullptr;
				EntityId id = -1;
				for (auto& iterId : env->editor->selectionContext->getSelected()) {
					if (env->world->hasComponent(iterId, desc->classId)) {
						comp = env->world->getComponent(iterId, desc->classId);
						id = iterId;
						break;
					}
				}
				if (comp) {
					multiEditComponent(id, desc->classId, comp, true);
				}
			}

			ImGui::Separator();
		}

		void singleEditComponent(EntityId id, int classId, void *comp, bool drawHidden) {
			//for undo system
			DynamicObjectBuffer preEdit;
			preEdit.set(classId, comp);

			bool change = env->editor->classUI->draw(classId, comp, nullptr, nullptr, nullptr, false, drawHidden);

			//for undo system
			if (change) {
				if (currentChangeClassId == -1) {
					currentChangeClassId = classId;
					singleEditPreChangeValue.set(classId, preEdit.get());
				}
			}
			if (currentChangeClassId == classId) {
				if (!change) {
					if (ImGui::IsAnyItemActive()) {
						change = true;
					}
				}
				if (!change) {
					env->editor->undo->componentChanged(classId, id, singleEditPreChangeValue.get());
					currentChangeClassId = -1;
					singleEditPreChangeValue.clear();
				}
			}
		}

		void singleEdit(EntityId id) {
			for (auto* desc : Reflection::getDescriptors()) {
				if (desc && desc->flags & ClassDescriptor::COMPONENT && !(desc->flags & ClassDescriptor::HIDDEN)) {
					if (env->world->hasComponent(id, desc->classId)) {
						ImGui::PushID(desc->classId);
						bool visible = true;
						if (ImGui::CollapsingHeader(desc->name.c_str(), &visible, ImGuiTreeNodeFlags_DefaultOpen)) {
							if (!visible) {
								env->editor->entityOperations->removeComponent(id, desc->classId);
							}
							componentMenu(id, desc->classId);
							void* comp = env->world->getComponent(id, desc->classId);
							singleEditComponent(id, desc->classId, comp, false);
						}
						else {
							componentMenu(id, desc->classId);
						}

						ImGui::PopID();
					}
				}
			}
		}

		void propagateComponentChange(int rootClassId, int classId, void* preEdit, void* postEdit, int offset) {
			auto* desc = Reflection::getDescriptor(classId);
			if (desc) {
				if (desc->properties.size() == 0) {
					if (desc->hasEquals()) {
						if (!desc->equals(preEdit, postEdit)) {
							EntityId id = -1;
							for (auto& iterId : env->editor->selectionContext->getSelected()) {
								if (env->world->hasComponent(iterId, rootClassId)) {
									void *comp = env->world->getComponent(iterId, rootClassId);
									if (postEdit != (uint8_t*)comp + offset) {
										desc->copy(postEdit, (uint8_t*)comp + offset);
									}
								}
							}
						}


					}
				}
				for (auto& prop : desc->properties) {
					propagateComponentChange(rootClassId, prop.type->classId, (uint8_t*)preEdit + prop.offset, (uint8_t*)postEdit + prop.offset, offset + prop.offset);
				}
			}
		}

		void multiEditComponent(EntityId id, int classId, void* comp, bool drawHidden) {
			//for undo system
			DynamicObjectBuffer preEdit;
			preEdit.set(classId, comp);

			bool change = env->editor->classUI->draw(classId, comp, nullptr, nullptr, nullptr, true, drawHidden);

			//for undo system
			if (change) {
				if (currentChangeClassId == -1) {
					//save pre change values
					currentChangeClassId = classId;
					multiEditPreChangeValues.clear();
					for (auto& iterId : env->editor->selectionContext->getSelected()) {
						if (env->world->hasComponent(iterId, classId)) {
							if (iterId == id) {
								multiEditPreChangeValues.push_back({ iterId, preEdit });
							}
							else {
								void* iterComp = env->world->getComponent(iterId, classId);
								DynamicObjectBuffer preChange;
								preChange.set(classId, iterComp);
								multiEditPreChangeValues.push_back({ iterId, preChange });
							}
						}
					}
				}
			}
			if (currentChangeClassId == classId) {
				if (!change) {
					if (ImGui::IsAnyItemActive()) {
						change = true;
					}
				}
				if (!change) {
					//add pre change values to the undo system
					env->editor->undo->beginAction();
					for (auto& preChange : multiEditPreChangeValues) {
						env->editor->undo->componentChanged(classId, preChange.first, preChange.second.get());
					}
					env->editor->undo->endAction();
					multiEditPreChangeValues.clear();
					currentChangeClassId = -1;
				}
			}

			if (change) {
				propagateComponentChange(classId, classId, preEdit.get(), comp, 0);
			}
		}

		void multiEdit() {
			for (auto* desc : Reflection::getDescriptors()) {
				if (desc && desc->flags & ClassDescriptor::COMPONENT && !(desc->flags & ClassDescriptor::HIDDEN)) {

					void* comp = nullptr;
					EntityId id = -1;
					for (auto& iterId : env->editor->selectionContext->getSelected()) {
						if (env->world->hasComponent(iterId, desc->classId)) {
							comp = env->world->getComponent(iterId, desc->classId);
							id = iterId;
							break;
						}
					}

					if (comp) {
						ImGui::PushID(desc->classId);
						bool visible = true;
						if (ImGui::CollapsingHeader(desc->name.c_str(), &visible, ImGuiTreeNodeFlags_DefaultOpen)) {
							if (!visible) {
								env->editor->undo->beginAction();
								for (auto& iterId : env->editor->selectionContext->getSelected()) {
									if (env->world->hasComponent(iterId, desc->classId)) {
										env->editor->entityOperations->removeComponent(iterId, desc->classId);
									}
								}
								env->editor->undo->endAction();
							}
							multiComponentMenu(desc->classId);

							multiEditComponent(id, desc->classId, comp, false);
							/*
							//for undo system
							DynamicObjectBuffer preEdit;
							preEdit.set(desc->classId, comp);

							bool change = env->editor->classUI->draw(desc->classId, comp);

							//for undo system
							if (change) {
								if (currentChangeClassId == -1) {
									//save pre change values
									currentChangeClassId = desc->classId;
									multiEditPreChangeValues.clear();
									for (auto& iterId : env->editor->selectionContext->getSelected()) {
										if (env->world->hasComponent(iterId, desc->classId)) {
											if (iterId == id) {
												multiEditPreChangeValues.push_back({ iterId, preEdit });
											}
											else {
												void* iterComp = env->world->getComponent(iterId, desc->classId);
												DynamicObjectBuffer preChange;
												preChange.set(desc->classId, iterComp);
												multiEditPreChangeValues.push_back({ iterId, preChange });
											}
										}
									}
								}
							}
							if (currentChangeClassId == desc->classId) {
								if (!change) {
									if (ImGui::IsAnyItemActive()) {
										change = true;
									}
								}
								if (!change) {
									//add pre change values to the undo system
									env->editor->undo->beginAction();
									for (auto &preChange : multiEditPreChangeValues) {
										env->editor->undo->componentChanged(desc->classId, preChange.first, preChange.second.get());
									}
									env->editor->undo->endAction();
									multiEditPreChangeValues.clear();
									currentChangeClassId = -1;
								}
							}

							if (change) {
								propagateComponentChange(desc->classId, desc->classId, preEdit.get(), comp, 0);
							}
							*/

						}
						else {
							multiComponentMenu(desc->classId);
						}
						ImGui::PopID();
					}

				}
			}
		}

		void contextMenu() {
			if (ImGui::BeginPopupContextWindow("context", ImGuiMouseButton_Right, false)) {
				if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations->getCopiedComponentClassId() != -1)) {
					env->editor->undo->beginAction();
					for (auto& id : env->editor->selectionContext->getSelected()) {
						env->editor->entityOperations->pastComponent(id);
					}
					env->editor->undo->endAction();
				}
				ImGui::EndPopup();
			}
		}

		void componentMenu(EntityId id, int classId) {
			if (ImGui::BeginPopupContextItem("context")) {
				if (ImGui::MenuItem("Remove")) {
					env->editor->entityOperations->removeComponent(id, classId);
				}
				if (ImGui::MenuItem("Copy")) {
					env->editor->entityOperations->copyComponent(id, classId);
				}
				if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations->getCopiedComponentClassId() != -1)) {
					env->editor->entityOperations->pastComponent(id);
				}
				ImGui::EndPopup();
			}
		}

		void multiComponentMenu(int classId) {
			if (ImGui::BeginPopupContextItem("context")) {
				if (ImGui::MenuItem("Remove")) {
					env->editor->undo->beginAction();
					for (auto& id : env->editor->selectionContext->getSelected()) {
						if (env->world->hasComponent(id, classId)) {
							env->editor->entityOperations->removeComponent(id, classId);
						}
					}
					env->editor->undo->endAction();
				}
				if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations->getCopiedComponentClassId() != -1)) {
					env->editor->undo->beginAction();
					for (auto& id : env->editor->selectionContext->getSelected()) {
						if (env->world->hasComponent(id, classId)) {
							env->editor->entityOperations->pastComponent(id);
						}
					}
					env->editor->undo->endAction();
				}
				ImGui::EndPopup();
			}
		}

	};
	TRI_SYSTEM(PropertiesWindow);

}