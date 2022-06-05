//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "window/UIManager.h"
#include "window/Window.h"
#include "render/objects/Material.h"
#include "engine/AssetManager.h"
#include <imgui/imgui.h>

namespace tri {

	class MaterialWindow : public UIWindow {
	public:
		Ref<Material> material;

		void init() override {
			env->systemManager->getSystem<UIManager>()->addWindow<MaterialWindow>("Materials");
		}

		void shutdown() override {
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Materials", &active)) {

					env->editor->classUI->enableReferenceEdit = false;
					env->editor->classUI->draw(material, "material");
					env->editor->classUI->enableReferenceEdit = true;
					if (ImGui::Button("Save")) {
						if (material) {
							auto file = env->assetManager->getFile(material);
							if (!file.empty()) {
								file = env->assetManager->searchFile(file);
								material->save(file);
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Save All")) {
						for (auto& file : env->assetManager->getAssetList(Reflection::getClassId<Material>())) {
							if (!file.empty()) {
								auto mat = env->assetManager->get<Material>(file);
								if (mat) {
									file = env->assetManager->searchFile(file);
									mat->save(file);
								}
							}
						}
					}
					ImGui::Separator();
					if (material) {
						env->editor->classUI->draw(*material.get());
					}
				}
				ImGui::End();
			}
		}
	};
	TRI_SYSTEM(MaterialWindow);

}