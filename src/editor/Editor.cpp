//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "Gizmos.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "core/JobManager.h"
#include "core/config.h"
#include "window/Window.h"
#include "engine/Serializer.h"
#include "window/Input.h"
#include "window/UIManager.h"
#include "engine/RuntimeMode.h"
#include "engine/Map.h"
#include "engine/Time.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "engine/Light.h"
#include "engine/MeshComponent.h"
#include "engine/AssetManager.h"
#include "engine/EntityInfo.h"
#include "render/objects/Mesh.h"
#include <imgui.h>
#include <imgui/imgui_internal.h>

#if TRI_WINDOWS
#include <windows.h>
#endif

namespace tri {

	TRI_SYSTEM_INSTANCE(Editor, env->editor);

	void Editor::init() {
		auto *job = env->jobManager->addJob("Editor");
		job->addSystem<Editor>();
		autoSaveListener = -1;
		playBufferListener = -1;

	}

	void Editor::startup() {
		autoSaveListener = env->eventManager->preShutdown.addListener([&]() {
			saveMap("autosave.tmap");
		});
		crashSaveListener = env->eventManager->onUnhandledException.addListener([&]() {
			if (lastUnhandledExceptionTime == -1 || env->time->time - lastUnhandledExceptionTime > 10.0f) {
				saveMap("crashsave.tmap");
			}
			lastUnhandledExceptionTime = env->time->time;
		});

		playBuffer = new World();

		playBufferListener = env->eventManager->onRuntimeModeChange.addListener([&](int prev, int mode) {
			env->eventManager->postTick.addListener([&, prev, mode]() {
				if (mode == RuntimeMode::PLAY && prev == RuntimeMode::EDIT) {
					playBuffer->copy(*env->world);
				}else if (mode == RuntimeMode::PAUSED && prev == RuntimeMode::EDIT) {
					playBuffer->copy(*env->world);
				}
				else if (mode == RuntimeMode::EDIT && prev != RuntimeMode::LOADING) {
					std::vector<std::pair<EntityId, Prefab>> persistent;

					for (auto& i : playModePersistentEntities) {
						Prefab p;
						p.copyEntity(i.second);
						persistent.push_back({ i.second, p });
					}

					env->world->copy(*playBuffer);

					for (auto& i : persistent) {
						i.second.copyIntoEntity(i.first);
					}
				}
			}, true);
			env->uiManager->updateActiveFlags();
		});

		setupIniHandler();
		if (ImGui::GetCurrentContext()) {
			ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);
		}

		env->runtimeMode->setMode(RuntimeMode::EDIT);
	}

	void Editor::shutdown() {
		env->eventManager->preShutdown.removeListener(autoSaveListener);
		env->eventManager->onUnhandledException.removeListener(crashSaveListener);
		env->eventManager->onRuntimeModeChange.removeListener(playBufferListener);
	}

	void Editor::tick() {
		if (env->window && env->window->inFrame()) {
			bool openAbout = false;
			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("New")) {
						env->world->clear();
						env->world->addEntity(EntityInfo("Camera"), 
							Transform({ -2.5, 2, 2 }, { 1, 1, 1 }, glm::radians(glm::vec3(-30.0f, -50.0f, 0.0f))),
							Camera(Camera::PERSPECTIVE, true));
						env->world->addEntity(EntityInfo("AmbientLight"), Transform(), AmbientLight());
						env->world->addEntity(EntityInfo("DirectionalLight"), 
							Transform({ 0, 0, 0 }, { 1, 1, 1 }, glm::radians(glm::vec3(10.0f, 25.0f, -50.0f))),
							DirectionalLight());
						env->world->addEntity(EntityInfo("Cube"), Transform(), MeshComponent(env->assetManager->get<Mesh>("models/cube.obj")));
					}
					if (ImGui::MenuItem("Open", "Ctrl+O")) {
						std::string file = openFileDialog(".tmap\0*.tmap\0.*\0*.*\0");
						if (!file.empty()) {
							env->eventManager->postTick.addListener([file]() {
								Map::loadAndSetToActiveWorld(file, RuntimeMode::EDIT);
							}, true);
						}
					}
					if (ImGui::MenuItem("Save", "Ctrl+S")) {
						env->eventManager->postTick.addListener([&]() {
							if (!env->worldFile.empty()) {
								saveMap(env->worldFile);
							}
						}, true);
					}
					if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
						std::string file = openFileDialog(".tmap\0*.tmap\0.*\0*.*\0", true);
						if (!file.empty()) {
							env->eventManager->postTick.addListener([&, file]() {
								saveMap(file);
								env->worldFile = file;
							}, true);
						}
					}


					if (ImGui::MenuItem("Save Binary")) {
						env->eventManager->postTick.addListener([]() {
							Clock clock;
							env->serializer->serializeWorldBinary(env->world, "world.bin");
							env->console->info("save (bin) world took %f s", clock.elapsed());
						}, true);
					
					}
					if (ImGui::MenuItem("Load Binary")) {
						env->eventManager->postTick.addListener([]() {
							Clock clock;
							env->serializer->deserializeWorldBinary(env->world, "world.bin");
							env->console->info("load (bin) world took %f s", clock.elapsed());
						}, true);
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Extra")) {
					if (ImGui::MenuItem("About")) {
						openAbout = true;
					}
					if (ImGui::MenuItem("Reset Layout")) {
						env->uiManager->resetLayout();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			if (openAbout) {
				ImGui::OpenPopup("About");
			}

			bool open = true;
			if (ImGui::BeginPopupModal("About", &open)) {
				ImGui::Text("Tridot Engine");
				ImGui::Text("Version %s", TRI_VERSION);
				ImGui::Text("By Julian Hinxlage");
				if (!open) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			//shortcuts
			if (env->input->downControl()) {
				//undo/redo
				if (env->input->pressed('Y')) {
					if (!env->input->downShift()) {
						undo->undo();
					}
					else {
						undo->redo();
					}
				}

				if (env->input->downShift()) {
					if (env->input->pressed('S')) {
						std::string file = openFileDialog(".tmap\0*.tmap\0.*\0*.*\0", true);
						if (!file.empty()) {
							env->eventManager->postTick.addListener([&, file]() {
								saveMap(file);
								env->worldFile = file;
							}, true);
						}
					}
				}
				else {
					if (env->input->pressed('S')) {
						env->eventManager->postTick.addListener([&]() {
							if (!env->worldFile.empty()) {
								saveMap(env->worldFile);
							}
						}, true);
					}			
				}

				if (env->input->pressed('O')) {
					std::string file = openFileDialog(".tmap\0*.tmap\0.*\0*.*\0");
					if (!file.empty()) {
						env->eventManager->postTick.addListener([file]() {
							Map::loadAndSetToActiveWorld(file, RuntimeMode::EDIT);
						}, true);
					}
				}

				if (env->input->pressed('C')) {
					if (env->editor->selectionContext->isSingleSelection()) {
						EntityId id = env->editor->selectionContext->getSelected()[0];
						env->editor->entityOperations->copyEntity(id);
					}
				}
				if (env->input->pressed('V')) {
					if (env->editor->entityOperations->hasCopiedEntity()) {
						env->editor->selectionContext->select(env->editor->entityOperations->pastEntity());
					}
				}
				if (env->input->pressed('D')) {
					env->editor->entityOperations->duplicateSelection();
				}
			}
			if (env->input->pressed(Input::KEY_DELETE)) {
				env->editor->undo->beginAction();
				for (EntityId id : env->editor->selectionContext->getSelected()) {
					env->editor->entityOperations->removeEntity(id);
				}
				env->editor->selectionContext->unselectAll();
				env->editor->undo->endAction();
			}
			if (env->input->pressed(Input::KEY_F5)) {
				auto mode = env->runtimeMode->getMode();
				if (mode == RuntimeMode::EDIT) {
					env->runtimeMode->setMode(RuntimeMode::PLAY);
				}
				else if (mode == RuntimeMode::PLAY) {
					env->runtimeMode->setMode(RuntimeMode::EDIT);
				}
				else if (mode == RuntimeMode::PAUSED) {
					env->runtimeMode->setMode(RuntimeMode::EDIT);
				}
			}
			if (env->input->pressed(Input::KEY_F6)) {
				auto mode = env->runtimeMode->getMode();
				if (mode == RuntimeMode::PAUSED) {
					env->runtimeMode->setMode(RuntimeMode::PLAY);
				}
				else if (mode == RuntimeMode::PLAY) {
					env->runtimeMode->setMode(RuntimeMode::PAUSED);
				}
				else if (mode == RuntimeMode::EDIT) {
					env->runtimeMode->setMode(RuntimeMode::PAUSED);
				}
			}

		}
	}

	int Editor::setPersistentEntity(EntityId id, int handle) {
		if (handle == -1) {
			handle = 0;
			while (true) {
				bool change = false;
				for (auto& i : playModePersistentEntities) {
					if (i.first == handle) {
						handle++;
						change = false;
						break;
					}
				}
				if (!change) {
					break;
				}
			}
			playModePersistentEntities.push_back({ handle, id });
			return handle;
		}
		else {
			for (int i = 0; i < playModePersistentEntities.size(); i++) {
				auto& entity = playModePersistentEntities[i];
				if (entity.first == handle) {
					if (id == -1) {
						playModePersistentEntities.erase(playModePersistentEntities.begin() + i);
						handle = -1;
					}
					else {
						entity.second = id;
					}
					break;
				}
			}
			return handle;
		}
	}

	std::string Editor::openFileDialog(const char *pattern, bool allowNewFile) {
		TRI_PROFILE_FUNC();
#if TRI_WINDOWS
		OPENFILENAME ofn = { 0 };
		TCHAR szFile[260] = { 0 };
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = (HWND)env->window->getNativeContext();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = pattern;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;

		if (allowNewFile) {
			ofn.Flags = OFN_NOCHANGEDIR;
		}
		else {
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		}

		if (GetOpenFileName(&ofn) == TRUE) {
			std::string file = ofn.lpstrFile;
			return file;
		}
		else {
			return "";
		}
#else
		return "";
#endif
	}

	void Editor::saveMap(const std::string& file) {
		if (env->runtimeMode->getMode() != RuntimeMode::EDIT) {
			env->serializer->serializeWorld(playBuffer, file);
		}
		else {
			env->serializer->serializeWorld(env->world, file);
		}
	}

	void Editor::setupIniHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "Editor";
		handler.TypeHash = ImHashStr("Editor");
		handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
			if (std::string(name) == "") {
				return (void*)1;
			}
			else {
				return nullptr;
			}
		};
		handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
			auto parts = StrUtil::split(line, "=");
			if (parts.size() >= 2) {
				auto* gizmos = env->systemManager->getSystem<Gizmos>();
				if (parts[0] == "CameraInPlay") {
					env->editor->viewportCameraInPlay = std::stoi(parts[1]);
				}
				else if (parts[0] == "GizmoOperation") {
					gizmos->operation = (Gizmos::Operation)std::stoi(parts[1]);
				}
				else if (parts[0] == "GizmoMode") {
					gizmos->mode = (Gizmos::Mode)std::stoi(parts[1]);
				}
				else if (parts[0] == "GizmoPivots") {
					gizmos->pivots = (Gizmos::Pivots)std::stoi(parts[1]);
				}
				else if (parts[0] == "GizmoSnapping") {
					gizmos->snapping = std::stoi(parts[1]);
				}

				else if (parts[0] == "GizmoSnapPosX") {
					gizmos->translateSnapValues.x = std::stof(parts[1]);
				}
				else if (parts[0] == "GizmoSnapPosY") {
					gizmos->translateSnapValues.y = std::stof(parts[1]);
				}
				else if (parts[0] == "GizmoSnapPosZ") {
					gizmos->translateSnapValues.z = std::stof(parts[1]);
				}

				else if (parts[0] == "GizmoSnapScaleX") {
					gizmos->scaleSnapValues.x = std::stof(parts[1]);
				}
				else if (parts[0] == "GizmoSnapScaleY") {
					gizmos->scaleSnapValues.y = std::stof(parts[1]);
				}
				else if (parts[0] == "GizmoSnapScaleZ") {
					gizmos->scaleSnapValues.z = std::stof(parts[1]);
				}

				else if (parts[0] == "GizmoSnapRotX") {
					gizmos->rotateSnapValues.x = std::stof(parts[1]);
				}
				else if (parts[0] == "GizmoSnapRotY") {
					gizmos->rotateSnapValues.y = std::stof(parts[1]);
				}
				else if (parts[0] == "GizmoSnapRotZ") {
					gizmos->rotateSnapValues.z = std::stof(parts[1]);
				}
			}
		};
		handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
			auto* ui = env->uiManager;
			buf->append("[Editor][]\n");
			buf->appendf("%s=%i\n", "CameraInPlay", (int)env->editor->viewportCameraInPlay);
			
			auto *gizmos = env->systemManager->getSystem<Gizmos>();
			buf->appendf("%s=%i\n", "GizmoOperation", (int)gizmos->operation);
			buf->appendf("%s=%i\n", "GizmoMode", (int)gizmos->mode);
			buf->appendf("%s=%i\n", "GizmoPivots", (int)gizmos->pivots);

			buf->appendf("%s=%i\n", "GizmoSnapping", (int)gizmos->snapping);
			buf->appendf("%s=%f\n", "GizmoSnapPosX", (float)gizmos->translateSnapValues.x);
			buf->appendf("%s=%f\n", "GizmoSnapPosY", (float)gizmos->translateSnapValues.y);
			buf->appendf("%s=%f\n", "GizmoSnapPosZ", (float)gizmos->translateSnapValues.z);
			buf->appendf("%s=%f\n", "GizmoSnapScaleX", (float)gizmos->scaleSnapValues.x);
			buf->appendf("%s=%f\n", "GizmoSnapScaleY", (float)gizmos->scaleSnapValues.y);
			buf->appendf("%s=%f\n", "GizmoSnapScaleZ", (float)gizmos->scaleSnapValues.z);
			buf->appendf("%s=%f\n", "GizmoSnapRotX", (float)gizmos->rotateSnapValues.x);
			buf->appendf("%s=%f\n", "GizmoSnapRotY", (float)gizmos->rotateSnapValues.y);
			buf->appendf("%s=%f\n", "GizmoSnapRotZ", (float)gizmos->rotateSnapValues.z);

			buf->appendf("\n");
		};
		if (ImGui::GetCurrentContext()) {
			ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
		}
	}
}
