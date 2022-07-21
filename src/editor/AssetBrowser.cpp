//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "window/UIManager.h"
#include "window/Window.h"
#include "engine/AssetManager.h"
#include "engine/Serializer.h"
#include "engine/Map.h"

#include "render/objects/Texture.h"
#include "render/objects/Shader.h"
#include "render/objects/Material.h"
#include "render/objects/Mesh.h"

#include <imgui/imgui.h>

namespace tri {

	class AssetBrowser : public UIWindow {
	public:
		std::unordered_map<std::string, int> fileAssosiations;

		void init() override {
			env->systemManager->addSystem<Editor>();
			env->systemManager->getSystem<UIManager>()->addWindow<AssetBrowser>("Asset Browser");
			fileAssosiations = {
				{".glsl", Reflection::getClassId<Shader>() },
				{".png", Reflection::getClassId<Texture>() },
				{".jpg", Reflection::getClassId<Texture>() },
				{".mat", Reflection::getClassId<Material>() },
				{".obj", Reflection::getClassId<Mesh>() },
				{".tmap", Reflection::getClassId<World>() },
			};
		}

		void file(const std::string& name, const std::string& path) {
			int id = -1;
			auto i = fileAssosiations.find(std::filesystem::path(name).extension().string());
			if (i != fileAssosiations.end()) {
				id = i->second;
			}
			if (id != -1) {

				if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf)) {

					if (id == Reflection::getClassId<World>()) {
						if (ImGui::BeginPopupContextItem()) {
							if (ImGui::MenuItem("Load")) {
								Map::loadAndSetToActiveWorld(path);
							}
							bool isLoaded = env->assetManager->getStatus(path) & AssetManager::Status::LOADED;
							if (ImGui::MenuItem("Unload", nullptr, nullptr, isLoaded)) {
								env->assetManager->unload(path);
							}
							ImGui::EndPopup();
						}
					}
					else {
						if (ImGui::BeginPopupContextItem()) {
							if (ImGui::MenuItem("Load")) {
								env->assetManager->get(id, path);
							}
							bool isLoaded = env->assetManager->getStatus(path) & AssetManager::Status::LOADED;
							if (ImGui::MenuItem("Unload", nullptr, nullptr, isLoaded)) {
								env->assetManager->unload(path);
							}
							ImGui::EndPopup();
						}
					}

					ImGui::TreePop();
				}
			}

			if (id != -1) {
				env->editor->classUI->dragSource(id, path);
			}
		}

		void directory(const std::string &dir, const std::string& path) {
			if (ImGui::TreeNodeEx(dir.c_str())) {
				for (auto& i : std::filesystem::directory_iterator(path)) {
					if (i.is_directory()) {
						directory(i.path().filename().string(), i.path().string());
					}
					else {
						file(i.path().filename().string(), i.path().string());
					}
				}
				ImGui::TreePop();
			}
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Asset Browser", &active)) {
					
					for (auto& dir : env->assetManager->getSearchDirectories()) {
						directory(std::filesystem::path(dir).parent_path().filename().string(), dir);
					}

				}
				ImGui::End();
			}
		}
	};
	TRI_SYSTEM(AssetBrowser);

}