//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/AssetManager.h"
#include <imgui/imgui.h>

namespace tri {

    class AssetBrowserWindow : public EditorWindow {
    public:

        void startup() {
            name = "Asset Browser";
        }

        void update() override {
            editor->gui.file.browse([&](const std::string &file, int typeId){
                if(typeId != -1) {
                    std::string path = env->assets->minimalFilePath(file);
                    editor->gui.dragDropSource(typeId, file);
                    if (typeId == env->reflection->getTypeId<Scene>()) {
                        updateSceneMenu(file);
                    } else {
                        updateAssetMenu(typeId, file);
                    }
                }
            });
        }

        void updateAssetMenu(int typeId, const std::string &file){
            AssetManager::Status status = env->assets->getStatus(file);
            if(ImGui::BeginPopupContextItem()){
                if(ImGui::MenuItem("Load", nullptr, false, status & AssetManager::UNLOADED)){
                    env->assets->get(typeId, file);
                }
                if(ImGui::MenuItem("Unload", nullptr, false, status & AssetManager::LOADED)){
                    env->assets->unload(file);
                }
                if(ImGui::MenuItem("Reload", nullptr, false, status & AssetManager::LOADED)){
                    env->assets->unload(file);
                    env->assets->get(typeId, file);
                }
                ImGui::EndPopup();
            }
        }

        void updateSceneMenu(const std::string &file){
            if(ImGui::BeginPopupContextItem()){
                if(ImGui::MenuItem("Load")){
                    Scene::loadMainScene(file);
                }
                ImGui::EndPopup();
            }
        }

    };
    TRI_STARTUP_CALLBACK("") {
        editor->addWindow(new AssetBrowserWindow);
    }

}
