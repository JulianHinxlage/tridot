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
#include "engine/MetaTypes.h"
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace tri {

	class EntityFilter {
	public:
		enum Type {
			NAME,
			COMPONENT,
			PROPERTY,
			SELECTION,
		};
		Type type;
		bool invert = false;
		bool active = true;
		std::string text = "";
		PropertyValueIdentifier property;
	};
	TRI_CLASS(EntityFilter);
	TRI_CLASS(EntityFilter::Type);
	TRI_ENUM4(EntityFilter::Type, NAME, COMPONENT, PROPERTY, SELECTION);

	class EntitiesWindow : public UIWindow {
	public:
		EntityId lastClicked = -1;

		std::vector<EntityId> list;
		std::set<EntityId> set;
		bool needListUpdate = true;

		int onEntityAddListener = -1;
		int onEntityRemoveListener = -1;

		std::string searchText = "";
		std::vector<EntityFilter> filters;

		void init() override {
			env->uiManager->addWindow<EntitiesWindow>("Entities");
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
				else if (value->type == EntityFilter::COMPONENT) {
					env->editor->classUI->draw(*((ComponentIdentifier*)&value->property), "");
				}
				else if (value->type == EntityFilter::PROPERTY) {
					env->editor->classUI->draw(value->property, "");
				}

				if (ImGui::Checkbox("active", &value->active)) {
					change = true;
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

						//if (needListUpdate) {
							updateList();
						//}

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
									env->world->addComponent<EntityInfo>(id).name = getUniqueEntityName(desc->name);
									env->world->addComponent(id, desc->classId);
									if (desc->classId != Reflection::getClassId<Transform>()) {
										env->world->addComponent<Transform>(id);
									}
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

			//draw full list for now
			//todo: fix
			start = 0;
			end = size;

			ImGui::SetCursorPosY(ImGui::GetCursorPos().y + spacing * start);
			for (uint32_t i = start; i < end; i++) {
				EntityId id = list[i];
				ImGui::PushID(id);

				bool show = true;
				auto* t = env->world->getComponent<Transform>(id);
				if (t) {
					if (t->parent != -1) {
						show = false;
					}
				}

				if (show) {
					entity(id, i);
				}

				ImGui::PopID();
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPos().y + spacing * (size - end));
		}

		bool isEntityInList(EntityId id) {
			return set.contains(id);
		}

		void updateList() {
			needListUpdate = false;
			list.clear();
			set.clear();
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

					if (filter.active) {

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
							if (filter.property.classId != -1) {
								if (!env->world->hasComponent(id, filter.property.classId)) {
									filterHit = true;
								}
							}
						}
						else if (filter.type == EntityFilter::PROPERTY) {

							if (filter.property.classId != -1) {
								auto* desc = Reflection::getDescriptor(filter.property.classId);
								if (desc) {

									if (auto *comp = env->world->getComponent(id, filter.property.classId)) {
										if (filter.property.propertyIndex >= 0 && filter.property.propertyIndex < desc->properties.size()) {
											auto &prop = desc->properties[filter.property.propertyIndex];
											filterHit = true;
											if (prop.type->equals((uint8_t*)comp + prop.offset, filter.property.value.get())) {
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
						else if (filter.type == EntityFilter::SELECTION) {
							if (!env->editor->selectionContext->isSelected(id)) {
								filterHit = true;
							}
						}

						if (filterHit == !filter.invert) {
							filterd = true;
						}
					}

				}

				if (!filterd) {
					list.push_back(id);
				}
			});
	
			for (auto id : list) {
				set.insert(id);
			}

			//add parents if not in list
			for (auto id : list) {
				auto* t = env->world->getComponent<Transform>(id);
				if (t && t->parent != -1) {
					if (!isEntityInList(t->parent)) {
						list.push_back(t->parent);
						set.insert(t->parent);
					}
				}
			}

			//sort by entity hierarchy
			std::vector<EntityId> newList;
			std::vector<EntityId> topLevelList;
			for (auto id : list) {
				auto* t = env->world->getComponent<Transform>(id);
				if (!t || t && t->parent == -1) {
					topLevelList.push_back(id);
				}
			}

			//sort alphabetical
			std::sort(topLevelList.begin(), topLevelList.end(), [](EntityId a, EntityId b) {
				if (auto* infoa = env->world->getComponent<EntityInfo>(a)) {
					if (auto* infob = env->world->getComponent<EntityInfo>(b)) {
						return infoa->name < infob->name;
					}
				}
				return a < b;
			});

			sortListByHierarchy(topLevelList, newList);
			list = newList;
			
			set.clear();
			for (auto id : list) {
				set.insert(id);
			}
		}

		void sortListByHierarchy(const std::vector<EntityId>&list, std::vector<EntityId> &newList) {
			for (auto id : list) {
				if (isEntityInList(id)) {
					auto childs = Transform::getChilds(id);

					//sort alphabetical
					std::sort(childs.begin(), childs.end(), [](EntityId a, EntityId b) {
						if (auto* infoa = env->world->getComponent<EntityInfo>(a)) {
							if (auto* infob = env->world->getComponent<EntityInfo>(b)) {
								return infoa->name < infob->name;
							}
						}
						return a < b;
					});

					newList.push_back(id);
					sortListByHierarchy(childs, newList);
				}
			}
		}

		void entity(EntityId id, int listIndex) {
			std::string lable = "<" + std::to_string(id) + ">";

			if (auto* info = env->world->getComponent<EntityInfo>(id)) {
				if (!info->name.empty()) {
					lable = info->name;
				}
			}

			auto& childs = Transform::getChilds(id);

			bool open = false;
			if (childs.size() != 0) {
				open = ImGui::TreeNodeEx("");
				ImGui::SameLine();
			}
			else {
				ImGui::TreePush();
				open = true;
			}

			if (ImGui::Selectable(lable.c_str(), env->editor->selectionContext->isSelected(id), ImGuiSelectableFlags_SpanAllColumns)) {
				selection(id);
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
				if (env->input->pressed(Input::MOUSE_BUTTON_LEFT)) {
					env->editor->dragEntityId = id;
				}
				if (env->input->released(Input::MOUSE_BUTTON_LEFT)) {
					if (env->editor->dragEntityId != id && env->editor->dragEntityId != -1) {
						env->editor->entityOperations->parentEntity(env->editor->dragEntityId, id);
						needListUpdate = true;
					}
					env->editor->dragEntityId = -1;
				}
			}

			entityContextMenu(id);

			if(open){
				for (auto& child : childs) {
					if (isEntityInList(child)) {
						ImGui::PushID(child);
						entity(child, listIndex);
						ImGui::PopID();
					}
				}
				ImGui::TreePop();
			}
		}

		void selection(EntityId id) {
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
				bool canParent = (env->editor->selectionContext->isSelected(id) && env->editor->selectionContext->isMultiSelection())
					|| !env->editor->selectionContext->isSelected(id);

				if (ImGui::MenuItem("Parent", nullptr, false, canParent)) {
					env->editor->undo->beginAction();
					auto selected = env->editor->selectionContext->getSelected();
					for (EntityId iterId : selected) {
						env->editor->entityOperations->parentEntity(iterId, id);
					}
					env->editor->undo->endAction();
				}
				if (ImGui::MenuItem("Unparent")) {
					if (env->editor->selectionContext->isSelected(id)) {
						env->editor->undo->beginAction();
						auto selected = env->editor->selectionContext->getSelected();
						for (EntityId id : selected) {
							env->editor->entityOperations->parentEntity(id, -1);
						}
						env->editor->undo->endAction();
					}
					else {
						env->editor->entityOperations->parentEntity(id, -1);
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