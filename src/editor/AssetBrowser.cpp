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
		std::mutex mutex;
		double checkTimeInterval = 4;

		class Node {
		public:
			std::string name;
			std::string path;
			bool isFile;
			std::vector<Node> childs;
		};
		std::vector<Node> nodes;

		void init() override {
			env->systemManager->addSystem<Editor>();
			env->uiManager->addWindow<AssetBrowser>("Asset Browser");
			
			env->editor->fileAssosiations[".glsl"] = Reflection::getClassId<Shader>();
			env->editor->fileAssosiations[".png"] = Reflection::getClassId<Texture>();
			env->editor->fileAssosiations[".jpg"] = Reflection::getClassId<Texture>();
			env->editor->fileAssosiations[".mat"] = Reflection::getClassId<Material>();
			env->editor->fileAssosiations[".obj"] = Reflection::getClassId<Mesh>();
			env->editor->fileAssosiations[".tmap"] = Reflection::getClassId<World>();
		}

		void updateTreeStep(Node &parent) {
			for (auto& i : std::filesystem::directory_iterator(parent.path)) {
				parent.childs.emplace_back();
				Node& node = parent.childs.back();
				node.name = i.path().filename().string();
				node.path = i.path().string();

				if (i.is_directory()) {
					node.isFile = false;
					updateTreeStep(node);
				}
				else {
					node.isFile = true;
				}
			}
		}

		void updateTree() {
			std::vector<Node> tmp;
			for (auto& path : env->assetManager->getSearchDirectories()) {
				tmp.emplace_back();
				Node& node = tmp.back();
				node.name = std::filesystem::path(path).parent_path().filename().string();
				node.path = path;
				node.isFile = false;

				updateTreeStep(node);
			}

			std::unique_lock<std::mutex> lock(mutex);
			nodes.clear();
			nodes = tmp;
		}

		void startup() {
			env->threadManager->addThread("File Tree Update", [&]() {
				while (true) {
					{
						TRI_PROFILE("FileTreeUpdate");
						updateTree();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds((long long)(checkTimeInterval * 1000.0)));
				}
			});
		}

		void file(const std::string& name, const std::string& path) {
			int id = -1;
			auto i = env->editor->fileAssosiations.find(std::filesystem::path(name).extension().string());
			if (i != env->editor->fileAssosiations.end()) {
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

		void step(Node &node) {
			if (node.isFile) {
				file(node.name, node.path);
			}
			else {
				if (ImGui::TreeNodeEx(node.name.c_str())) {
					for (auto& child : node.childs) {
						step(child);
					}
					ImGui::TreePop();
				}
			}
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Asset Browser", &active)) {
					std::unique_lock<std::mutex> lock(mutex);
					for (auto& node : nodes) {
						step(node);
					}
				}
				ImGui::End();
			}
		}
	};
	TRI_SYSTEM(AssetBrowser);

}