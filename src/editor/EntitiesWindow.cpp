//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "window/UIManager.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "window/Window.h"
#include "window/Input.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "engine/Random.h"
#include "engine/Time.h"
#include "engine/EntityInfo.h"
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace tri {

	class EntityFilter {
	public:
		enum Type {
			NAME,
			COMPONENT,
			PROPERTY,
		};
		Type type;
		bool invert = false;
		std::string text = "";
		int classId = -1;
		int propertyIndex = -1;
		DynamicObjectBuffer propertyValue;
	};
	TRI_CLASS(EntityFilter);
	TRI_CLASS(EntityFilter::Type);
	TRI_ENUM3(EntityFilter::Type, NAME, COMPONENT, PROPERTY);

	class EntitiesWindow : public UIWindow {
	public:
		EntityId lastClicked = -1;

		std::vector<EntityId> list;
		bool needListUpdate = true;

		int onEntityAddListener = -1;
		int onEntityRemoveListener = -1;

		std::string searchText = "";
		std::vector<EntityFilter> filters;

		void init() override {
			env->systemManager->getSystem<UIManager>()->addWindow<EntitiesWindow>("Entities");
			onEntityAddListener = env->eventManager->onEntityAdd.addListener([&](World *world, EntityId id) {
				needListUpdate = true;
			});
			onEntityRemoveListener = env->eventManager->onEntityRemove.addListener([&](World* world, EntityId id) {
				needListUpdate = true;
			});

			addFilterClassUI();
		}

		void addFilterClassUI() {
			env->editor->classUI->addClassUI<EntityFilter>([](const char* label, EntityFilter* value, EntityFilter* min, EntityFilter* max, bool multiValue) {
				bool change = false;
				change |= env->editor->classUI->draw(Reflection::getClassId<decltype(value->type)>(), &value->type, "type");
				if (value->type == EntityFilter::NAME) {
					change |= env->editor->classUI->draw(Reflection::getClassId<decltype(value->text)>(), &value->text, "name");
				}
				else if (value->type == EntityFilter::COMPONENT || value->type == EntityFilter::PROPERTY) {

					std::string preview = "";
					if (value->classId != -1) {
						if (auto* desc = Reflection::getDescriptor(value->classId)) {
							preview = desc->name;
						}
					}
					if (ImGui::BeginCombo("component", preview.c_str())) {
						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->flags & ClassDescriptor::COMPONENT && !(desc->flags & ClassDescriptor::HIDDEN)) {
								if (ImGui::Selectable(desc->name.c_str(), preview == desc->name)) {
									value->classId = desc->classId;
									change = true;
								}
							}
						}
						ImGui::EndCombo();
					}


					if (value->type == EntityFilter::PROPERTY) {
						if (value->classId != -1) {
							if (auto* desc = Reflection::getDescriptor(value->classId)) {
								std::string preview = "";
								if (value->propertyIndex >= 0 && value->propertyIndex < desc->properties.size()) {
									preview = desc->properties[value->propertyIndex].name;
								}
								if (ImGui::BeginCombo("property", preview.c_str())) {
									for (int i = 0; i < desc->properties.size(); i++) {
										auto& prop = desc->properties[i];
										if (!(prop.flags & PropertyDescriptor::HIDDEN)) {
											if (ImGui::Selectable(prop.name.c_str(), preview == prop.name)) {
												value->propertyIndex = i;
												change = true;
											}
										}
									}
									ImGui::EndCombo();
								}

								if (value->propertyIndex >= 0 && value->propertyIndex < desc->properties.size()) {
									auto &prop =desc->properties[value->propertyIndex];
									if (value->propertyValue.classId != prop.type->classId) {
										value->propertyValue.clear();
										value->propertyValue.set(prop.type->classId);
									}
									change |= env->editor->classUI->draw(prop.type->classId, value->propertyValue.get(), "value");
								}


							}
						}
					}

				}

				if (ImGui::Checkbox("invert", &value->invert)) {
					change = true;
				}

				return change;
			});
		}

		void shutdown() {
			env->eventManager->onEntityAdd.removeListener(onEntityAddListener);
			env->eventManager->onEntityRemove.removeListener(onEntityRemoveListener);
			filters.clear();
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Entities", &active)) {
					if (env->world) {
						header();

						if (needListUpdate) {
							updateList();
						}

						if (ImGui::BeginChild("child")) {
							entityList();
							contextMenu();
							ImGui::EndChild();
						}

					}

				}
				ImGui::End();
			}
		}

		std::string getUniqueEntityName(std::string name) {
			bool change = false;
			env->world->each<const EntityInfo>([&](EntityInfo &info) {
				if (name == info.name) {
					change = true;
					auto parts = StrUtil::split(name, " ", true);
					if (parts.size() > 0) {
						int num = 1;
						bool hasNum = false;
						try {
							num = std::stoi(parts[parts.size() - 1]);
							hasNum = true;
						}
						catch (...) {}

						if (hasNum) {
							parts.pop_back();
							name = StrUtil::join(parts, " ");
							name += " " + std::to_string(num + 1);
						}
						else {
							name = name + " 1";
						}
					}
					else {
						name = name + " 1";
					}
				}
			});
			if (change) {
				return getUniqueEntityName(name);
			}
			else {
				return name;
			}
		}

		void header() {
			if (ImGui::Button("Add Entity")) {
				ImGui::OpenPopup("add");
			}
			
			if (ImGui::BeginPopup("add")) {

				if (ImGui::BeginMenu("Component")) {
					for (auto* desc : Reflection::getDescriptors()) {
						if (desc && desc->flags & ClassDescriptor::COMPONENT && !(desc->flags & ClassDescriptor::HIDDEN)) {
							if (desc->category.empty() || ImGui::BeginMenu(desc->category.c_str())) {
								if (ImGui::MenuItem(desc->name.c_str())) {
									EntityId id = env->world->addEntity();
									env->world->addComponent(id, desc->classId);
									env->world->addComponent<EntityInfo>(id).name = getUniqueEntityName(desc->name);
									env->editor->selectionContext->select(id);
									env->editor->undo->entityAdded(id);
								}
								if (!desc->category.empty()) {
									ImGui::EndMenu();
								}
							}
						}
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Empty")) {
					EntityId id = env->world->addEntity();
					env->world->addComponent<EntityInfo>(id).name = getUniqueEntityName("Entity");
					env->editor->selectionContext->select(id);
					env->editor->undo->entityAdded(id);
				}

				ImGui::EndPopup();
			}

			//filtering: text name search, component, property value
			//multiple filter: AND and OR
			if (ImGui::InputText("Search", &searchText)) {
				needListUpdate = true;
			}
			if (ImGui::TreeNode("filter")) {
				if (env->editor->classUI->draw(Reflection::getClassId<decltype(filters)>(), &filters)) {
					needListUpdate = true;
				}
				if (ImGui::Button("Update Filter")) {
					needListUpdate = true;
				}
				ImGui::TreePop();
			}

			ImGui::Separator();
		}

		void entityList() {
			int size = list.size();

			float spacing = ImGui::GetStyle().IndentSpacing;
			int start = (ImGui::GetScrollY() - ImGui::GetCursorPos().y) / spacing;
			int end = start + ImGui::GetWindowSize().y * 1.25f / spacing;
			end += 2;
			end = std::min(end, size);
			start = std::max(start, 0);

			ImGui::SetCursorPosY(ImGui::GetCursorPos().y + spacing * start);
			for (uint32_t i = start; i < end; i++) {
				EntityId id = list[i];
				ImGui::PushID(id);

				entity(id);

				ImGui::PopID();
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPos().y + spacing * (size - end));
		}

		void updateList() {
			needListUpdate = false;
			list.clear();
			env->world->each<>([&](EntityId id) {
				bool filterd = false;

				//search
				if (!searchText.empty()) {
					if (auto* info = env->world->getComponent<EntityInfo>(id)) {
						if (StrUtil::isSubstring(info->name, searchText) == 0) {
							filterd = true;
						}
					}
					else {
						filterd = true;
					}
				}

				for (auto& filter : filters) {
					bool filterHit = false;

					if (filter.type == EntityFilter::NAME) {
						if (!filter.text.empty()) {
							if (auto* info = env->world->getComponent<EntityInfo>(id)) {
								if (StrUtil::isSubstring(info->name, filter.text) == 0) {
									filterHit = true;
								}
							}
							else {
								filterHit = true;
							}
						}
					}
					else if (filter.type == EntityFilter::COMPONENT) {
						if (filter.classId != -1) {
							if (!env->world->hasComponent(id, filter.classId)) {
								filterHit = true;
							}
						}
					}
					else if (filter.type == EntityFilter::PROPERTY) {

						if (filter.classId != -1) {
							auto* desc = Reflection::getDescriptor(filter.classId);
							if (desc) {

								if (auto *comp = env->world->getComponent(id, filter.classId)) {
									if (filter.propertyIndex >= 0 && filter.propertyIndex < desc->properties.size()) {
										auto &prop = desc->properties[filter.propertyIndex];
										filterHit = true;
										if (prop.type->equals((uint8_t*)comp + prop.offset, filter.propertyValue.get())) {
											filterHit = false;
										}
									}
									else {
										filterHit = false;
									}
								}
								else {
									filterHit = true;
								}


							}
						}

					}

					if (filterHit == !filter.invert) {
						filterd = true;
					}

				}

				if (!filterd) {
					list.push_back(id);
				}
			});

			//sort
			std::sort(list.begin(), list.end(), [](EntityId a, EntityId b) {
				if (auto* infoa = env->world->getComponent<EntityInfo>(a)) {
					if (auto* infob = env->world->getComponent<EntityInfo>(b)) {
						return infoa->name < infob->name;
					}
				}
				return a < b;
			});
		}

		void entity(EntityId id) {
			std::string lable = "<" + std::to_string(id) + ">";

			if (auto* info = env->world->getComponent<EntityInfo>(id)) {
				if (!info->name.empty()) {
					lable = info->name;
				}
			}

			if (ImGui::Selectable(lable.c_str(), env->editor->selectionContext->isSelected(id))) {

				if (env->input->downShift()) {

					//range select
					if (lastClicked != -1 && id != lastClicked) {
						bool select = env->editor->selectionContext->isSelected(lastClicked);
						bool inRange = false;
						for (uint32_t i = 0; i < list.size(); i++) {
							EntityId iterId = list[i];

							bool iterInRange = inRange;
							if (iterId == id || iterId == lastClicked) {
								inRange = !inRange;
								if (inRange) {
									iterInRange = true;
								}
							}

							if (iterInRange) {
								if (select) {
									env->editor->selectionContext->select(iterId, false);
								}
								else {
									env->editor->selectionContext->unselect(iterId);
								}
							}

						}
					}

				}
				else {
					//select
					if (env->editor->selectionContext->isSelected(id)) {
						if (env->input->downControl()) {
							env->editor->selectionContext->unselect(id);
						}
						else {
							env->editor->selectionContext->select(id, true);
						}
					}
					else {
						env->editor->selectionContext->select(id, !env->input->downControl());
					}
				}
				lastClicked = id;
			}

			entityContextMenu(id);
		}

		void entityContextMenu(EntityId id) {
			if (ImGui::BeginPopupContextItem("context")) {
				if (ImGui::MenuItem("Remove", "Del")) {

					if (env->editor->selectionContext->isSelected(id)) {
						env->editor->undo->beginAction();
						for (EntityId id : env->editor->selectionContext->getSelected()) {
							env->editor->entityOperations->removeEntity(id);
						}
						env->editor->selectionContext->unselectAll();
						env->editor->undo->endAction();
					}
					else {
						env->editor->entityOperations->removeEntity(id);
					}

				}
				bool canCopy = !env->editor->selectionContext->isMultiSelection() || !env->editor->selectionContext->isSelected(id);
				if (ImGui::MenuItem("Copy", "Ctrl+C", false, canCopy)) {
					env->editor->entityOperations->copyEntity(id);
				}
				if (ImGui::MenuItem("Past", "Ctrl+V", false, env->editor->entityOperations->hasCopiedEntity())) {
					env->editor->selectionContext->select(env->editor->entityOperations->pastEntity());
				}
				if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {

					if (env->editor->selectionContext->isSelected(id)) {
						env->editor->undo->beginAction();
						auto selected = env->editor->selectionContext->getSelected();
						env->editor->selectionContext->unselectAll();
						for (EntityId id : selected) {
							env->editor->selectionContext->select(env->editor->entityOperations->duplicateEntity(id), false);
						}
						env->editor->undo->endAction();
					}
					else {
						env->editor->selectionContext->select(env->editor->entityOperations->duplicateEntity(id));
					}

				}
				ImGui::EndPopup();
			}
		}

		void contextMenu() {
			if (ImGui::BeginPopupContextWindow("context", ImGuiMouseButton_Right, false)) {
				if (ImGui::MenuItem("Past", "Ctrl+V", false, env->editor->entityOperations->hasCopiedEntity())) {
					env->editor->selectionContext->select(env->editor->entityOperations->pastEntity());
				}
				ImGui::EndPopup();
			}
		}
	};
	TRI_SYSTEM(EntitiesWindow);

}