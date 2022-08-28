//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "window/UIManager.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "window/Window.h"
#include "entity/World.h"
#include "engine/Transform.h"
#include "engine/Random.h"
#include "engine/Time.h"
#include <imgui/imgui.h>

namespace tri {

	class EntitiesInfoWindow : public UIWindow {
	public:
		void init() override {
			env->uiManager->addWindow<EntitiesInfoWindow>("Entities Info", "Debug");
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Entities Info", &active)) {

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
									}
									ImGui::TreePop();
								}


							}
						}

					}

				}
				ImGui::End();
			}
		}
	};
	TRI_SYSTEM(EntitiesInfoWindow);

}
