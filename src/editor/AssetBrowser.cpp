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
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace tri {

	class AssetBrowser : public UIWindow {
	public:
		std::mutex mutex;
		double checkTimeInterval = 4;
		bool needsUpdateTree = false;
		int updateTreeThreadId = -1;
		int updateTreeThreadRunning = false;

		class Node {
		public:
			std::string name;
			std::string path;
			bool isFile;
			std::vector<Node> childs;
		};
		std::vector<Node> nodes;
		

		class TextBox {
		public:
			enum Mode {
				FILE,
				FOLDER,
				RENAME,
			};

			Mode mode;
			bool open;
			bool openLast;
			std::string text;
			std::string original;
			std::string directory;
			int newFileClassId;
		};
		TextBox textBox;

		void init() override {
			env->systemManager->addSystem<Editor>();
			env->uiManager->addWindow<AssetBrowser>("Asset Browser");
			
			env->editor->fileAssosiations[".glsl"] = Reflection::getClassId<Shader>();
			env->editor->fileAssosiations[".png"] = Reflection::getClassId<Texture>();
			env->editor->fileAssosiations[".jpg"] = Reflection::getClassId<Texture>();
			env->editor->fileAssosiations[".mat"] = Reflection::getClassId<Material>();
			env->editor->fileAssosiations[".obj"] = Reflection::getClassId<Mesh>();
			env->editor->fileAssosiations[".tmap"] = Reflection::getClassId<Map>();
			env->editor->fileAssosiations[".prefab"] = Reflection::getClassId<Prefab>();
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
			updateTreeThreadRunning = true;
			updateTreeThreadId = env->threadManager->addThread("File Tree Update", [&]() {
				while (updateTreeThreadRunning) {
					{
						TRI_PROFILE("FileTreeUpdate");
						updateTree();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds((long long)(checkTimeInterval * 1000.0)));
				}
			});
		}

		void shutdown() {
			updateTreeThreadRunning = false;
			env->threadManager->joinThread(updateTreeThreadId);
			env->threadManager->terminateThread(updateTreeThreadId);
		}

		void file(const std::string& name, const std::string& path) {
			int id = -1;
			auto i = env->editor->fileAssosiations.find(std::filesystem::path(name).extension().string());
			if (i != env->editor->fileAssosiations.end()) {
				id = i->second;
			}
			if (id != -1) {

				if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf)) {

					if (id == Reflection::getClassId<Map>()) {
						if (ImGui::BeginPopupContextItem()) {
							if (ImGui::MenuItem("Load")) {
								Map::loadAndSetToActiveWorld(path, RuntimeMode::EDIT);
							}
							bool isLoaded = env->assetManager->getStatus(path) & AssetManager::Status::LOADED;
							if (ImGui::MenuItem("Unload", nullptr, nullptr, isLoaded)) {
								env->assetManager->unload(path);
							}
							ImGui::EndPopup();
						}
					}
					else if (id == Reflection::getClassId<Prefab>()) {
						if (ImGui::BeginPopupContextItem()) {
							if (ImGui::MenuItem("Add Entity")) {
								auto prefab = env->assetManager->get<Prefab>(path, AssetManager::Options::SYNCHRONOUS);
								if (prefab) {
									std::map<EntityId, EntityId> idMap;
									EntityId id = prefab->createEntity(env->world, -1, &idMap);
									env->editor->undo->beginAction();
									for (auto &i : idMap) {
										env->editor->undo->entityAdded(i.second);
										env->editor->selectionContext->select(i.second);
									}
									env->editor->undo->endAction();
								}
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

					if (ImGui::BeginPopupContextItem()) {
						if (ImGui::MenuItem("Rename")) {
							textBox.mode = TextBox::RENAME;
							textBox.open = true;
							textBox.text = name;
							textBox.original = name;
							textBox.directory = std::filesystem::path(path).parent_path().string();
							textBox.newFileClassId = -1;
						}
						if (ImGui::MenuItem("Delete")) {
							//todo: are your sure dialog
							try {
								std::filesystem::remove(path);
							}
							catch (...) {}
							needsUpdateTree = true;
						}
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				}
			}

			if (id != -1) {
				env->editor->classUI->dragSource(id, path);
			}
		}

		void updateTextBox() {
			if (!textBox.openLast) {
				ImGui::SetKeyboardFocusHere();
			}
			if (ImGui::InputText("name", &textBox.text, ImGuiInputTextFlags_EnterReturnsTrue)) {
				std::string file = textBox.directory + "/" + textBox.text;

				if (textBox.mode == TextBox::FILE) {
					if (!std::filesystem::exists(file)) {
						env->assetManager->get(textBox.newFileClassId, file, (AssetManager::Options)(AssetManager::SYNCHRONOUS | AssetManager::DO_NOT_LOAD))->save(file);
						needsUpdateTree = true;
					}
				}
				else if (textBox.mode == TextBox::FOLDER) {
					if (!std::filesystem::exists(file)) {
						std::filesystem::create_directories(file);
						needsUpdateTree = true;
					}
				}
				else if (textBox.mode == TextBox::RENAME) {
					try {
						std::filesystem::rename(textBox.directory + "/" + textBox.original, file);
					}
					catch (...) {}
					needsUpdateTree = true;
				}
				textBox.open = false;
			}
			if (ImGui::IsItemDeactivated()) {
				textBox.open = false;
			}
			textBox.openLast = textBox.open;
		}

		void directoryMenu(Node& node) {
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::BeginMenu("New File")) {

					for (auto& assosiation : env->editor->fileAssosiations) {
						if (auto* desc = Reflection::getDescriptor(assosiation.second)) {
							if (ImGui::MenuItem(desc->name.c_str())) {
								textBox.mode = TextBox::FILE;
								textBox.open = true;
								textBox.text = assosiation.first;
								textBox.directory = node.path;
								textBox.newFileClassId = assosiation.second;
							}
						}
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("New Folder")) {
					textBox.mode = TextBox::FOLDER;
					textBox.open = true;
					textBox.text = "";
					textBox.directory = node.path;
					textBox.newFileClassId = -1;
				}

				if (ImGui::MenuItem("Delete")) {
					//todo: are your sure dialog
					try {
						std::filesystem::remove_all(node.path);
					}
					catch (...) {}
					needsUpdateTree = true;
				}

				ImGui::EndPopup();
			}
		}

		void step(Node &node) {
			if (node.isFile) {
				file(node.name, node.path);
			}
			else {
				if (ImGui::TreeNodeEx(node.name.c_str())) {
					directoryMenu(node);
					for (auto& child : node.childs) {
						step(child);
					}

					if (textBox.open && textBox.directory == node.path) {
						updateTextBox();
					}

					ImGui::TreePop();
				}
				else {
					directoryMenu(node);
				}
			}
		}

		void tick() override {
			needsUpdateTree = false;
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Asset Browser", &active)) {
					std::unique_lock<std::mutex> lock(mutex);
					for (auto& node : nodes) {
						step(node);
					}
				}
				ImGui::End();
			}
			if (needsUpdateTree) {
				updateTree();
			}
		}
	};
	TRI_SYSTEM(AssetBrowser);

}