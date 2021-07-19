//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ResourceBrowser.h"
#include "tridot/util/StrUtil.h"
#include "Editor.h"
#include "tridot/engine/Engine.h"
#include "tridot/engine/Plugin.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>

namespace tridot {

    void ResourceBrowser::init() {
        addFileAssociation<Texture>({".png", ".jpg"});
        addFileAssociation<Mesh>({".obj"});
        addFileAssociation<Scene>({".yml"});
        addFileAssociation<Shader>({".glsl"});
        addFileAssociation<Plugin>({".so", ".dll"});
    }

    void ResourceBrowser::update() {
        EditorGui::window("Resource Browser", [this](){
            for(auto &directory : env->resources->getSearchDirectories()){
                if(!isSubdirectory(directory)){
                    updateDirectory(directory, directory);
                }
            }
        });
    }

    void ResourceBrowser::updateDirectory(const std::string &directory, const std::string &baseDirectory) {
        std::string folder;
        if(!directory.empty() && directory.back() == '/'){
            folder = directory.substr(0, directory.size() - 1);
            folder = folder.substr(folder.find_last_of('/') + 1);
        }else{
            folder = directory.substr(directory.find_last_of('/') + 1);
        }

        if(ImGui::TreeNode(folder.c_str())) {
            std::vector<std::string> files;
            std::vector<std::string> subDirectories;

            for (auto &entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_directory()) {
                    subDirectories.push_back(entry.path().string());
                } else {
                    files.push_back(entry.path().string());
                }
            }

            std::sort(subDirectories.begin(), subDirectories.end());
            std::sort(files.begin(), files.end());

            for(auto &subDirectory : subDirectories){
                updateDirectory(subDirectory, baseDirectory);
            }

            for(auto &file : files){
                std::string name = std::filesystem::path(file).filename();
                std::string extension = std::filesystem::path(file).extension();

                //relative path to baseDirectory
                std::string path = file.substr(StrUtil::match(baseDirectory, file));

                if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf)) {
                    if(fileAssociations.contains(extension)){
                        fileAssociations[extension].dragDropSource(path);
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::Selectable("load")) {
                                fileAssociations[extension].load(path);
                            }
                            if (ImGui::Selectable("unload")) {
                                fileAssociations[extension].unload(path);
                            }
                            ImGui::EndPopup();
                        }
                    }
                    ImGui::TreePop();
                }

            }
            ImGui::TreePop();
        }
    }

    bool ResourceBrowser::isSubdirectory(const std::string &directory) {
        for(auto &dir : env->resources->getSearchDirectories()){
            if(dir.size() < directory.size()){
                if(StrUtil::match(dir, directory) == dir.size()){
                    return true;
                }
            }
        }
        return false;
    }

}
