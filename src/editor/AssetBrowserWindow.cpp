//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/AssetManager.h"
#include "render/Material.h"
#include <imgui/imgui.h>

namespace tri {

    class AssetBrowserWindow : public EditorWindow {
    public:
        std::unordered_map<std::string , int> associations;

        void startup() {
            name = "Asset Browser";

            associations = {
                    {".obj", env->reflection->getTypeId<Mesh>()},
                    {".scene", env->reflection->getTypeId<Scene>()},
                    {".glsl", env->reflection->getTypeId<Shader>()},
                    {".png", env->reflection->getTypeId<Texture>()},
                    {".jpg", env->reflection->getTypeId<Texture>()},
                    {".mat", env->reflection->getTypeId<Material>()},
            };
        }

        void update() override {
            for(auto &dir : env->assets->getSearchDirectories()){
                std::string name = std::filesystem::path(dir).parent_path().filename();
                if(ImGui::TreeNode(name.c_str())){
                    updateDirectory(dir, dir);
                    ImGui::TreePop();
                }
            }
        }

        void updateDirectory(const std::string &directory, const std::string &searchDirectory){
            for(auto &entry : std::filesystem::directory_iterator(directory)){
                if(entry.is_directory()){
                    if(ImGui::TreeNode(entry.path().filename().c_str())){
                        updateDirectory(entry.path(), searchDirectory);
                        ImGui::TreePop();
                    }
                }
            }for(auto &entry : std::filesystem::directory_iterator(directory)){
                if(entry.is_regular_file()){
                    if(ImGui::TreeNodeEx(entry.path().filename().c_str(), ImGuiTreeNodeFlags_Leaf)){
                        ImGui::TreePop();
                    }
                    auto x = associations.find(entry.path().extension());
                    if(x != associations.end()){
                        int typeId = x->second;
                        std::string file = entry.path().string().substr(searchDirectory.size());
                        editor->gui.dragDropSource(typeId, file);
                        if(typeId == env->reflection->getTypeId<Scene>()){
                            updateSceneMenu(file);
                        }else{
                            updateAssetMenu(typeId, file);
                        }
                    }
                }
            }
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
