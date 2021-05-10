//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ResourceBrowser.h"
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
            for(auto &directory : engine.resources.getSearchDirectories()){
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
                std::string path = file.substr(stringMatch(baseDirectory, file));

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
        for(auto &dir : engine.resources.getSearchDirectories()){
            if(dir.size() < directory.size()){
                if(stringMatch(dir, directory) == dir.size()){
                    return true;
                }
            }
        }
        return false;
    }

    int ResourceBrowser::stringMatch(const std::string &v1, const std::string &v2) {
        int match = 0;
        int size = std::min(v1.size(), v2.size());
        for(int i = 0; i < size; i++){
            if(v1[i] == v2[i]){
                match++;
            }else{
                break;
            }
        }
        return match;
    }

}