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

	class EntitiesInfoWindow : public System {
	public:
		bool& active() {
			return env->systemManager->getSystemHandle(Reflection::getClassId<EntitiesInfoWindow>())->active;
		}

		void init() override {
			env->editor->addWindow<EntitiesInfoWindow>("Entities Info", "Debug");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Entities Info", &active())) {

					if (env->world) {

						ImGui::Text("Entities:   %i", env->world->getEntityStorage()->size());
						int totalComponentCount = 0;
						float totalMemoryUsage = 0;

						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->flags & ClassDescriptor::COMPONENT) {
								auto* store = env->world->getComponentStorage(desc->classId);
								if (store) {
									totalComponentCount += store->size();
									float memoryUsage = (float)store->memoryUsage() / 1000.0f / 1000.0f;
									totalMemoryUsage += memoryUsage;
								}
							}
						}
						ImGui::Text("Components: %i", totalComponentCount);
						ImGui::Text("MB total:   %f", totalMemoryUsage);

						ImGui::Separator();

						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->flags & ClassDescriptor::COMPONENT) {
								if (ImGui::TreeNode(desc->name.c_str())) {
									auto* store = env->world->getComponentStorage(desc->classId);
									if (store) {
										float memoryUsage = (float)store->memoryUsage() / 1000.0f / 1000.0f;
										ImGui::Text("count:    %i", store->size());
										ImGui::Text("MB data:  %f", (float)(store->size() * desc->size) / 1000.0f / 1000.0f);
										ImGui::Text("MB total: %f", memoryUsage);

										if (ImGui::Button("Remove 50\%")) {
											env->world->view<>().each([&](EntityId id) {
												if (env->world->hasComponent(id, desc->classId)) {
													if (env->random->getFloat() < 0.5) {
														env->world->removeComponent(id, desc->classId);
													}
												}
											});
										}
									}
									ImGui::TreePop();
								}


							}
						}

						ImGui::Separator();

						if (ImGui::Button("Remove 50\% of Entities")) {
							env->world->view<>().each([&](EntityId id) {
								if (env->random->getFloat() < 0.5) {
									env->world->removeEntity(id);
								}
								});
						}

						if (ImGui::Button("Add 1000 Entities")) {
							for (int i = 0; i < 1000; i++) {
								auto id = env->world->addEntity();
								env->world->addComponent<Transform>(id);
							}
						}

						ImGui::Separator();

						for (auto* desc : Reflection::getDescriptors()) {
							if (desc && desc->flags & ClassDescriptor::COMPONENT) {
								auto* store = env->world->getComponentStorage(desc->classId);
								if (store) {
									ImGui::PushID(desc->classId);
									if (ImGui::TreeNode(desc->name.c_str())) {

										//if (store->alignementMaster) {
										//	ImGui::Text("alignement to master %s", Reflection::getDescriptor(store->alignementMaster->classId)->name.c_str());
										//	ImGui::Text("alignement count %i", store->alignementMasterSize);
										//}
										//if (store->alignementSlave) {
										//	ImGui::Text("alignement to slave %s", Reflection::getDescriptor(store->alignementSlave->classId)->name.c_str());
										//	ImGui::Text("alignement count %i", store->alignementSlaveSize);
										//}

										int size = std::min(1000, store->size());
										for (int i = 0; i < size; i++) {
											auto id = store->getIdByIndex(i);
											ImGui::Text("%i", id);
										}
										ImGui::TreePop();
									}
									ImGui::PopID();
								}
							}
						}

					}

					ImGui::End();
				}
			}
		}
	};
	TRI_SYSTEM(EntitiesInfoWindow);

}
