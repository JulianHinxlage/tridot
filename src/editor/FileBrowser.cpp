//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "FileBrowser.h"
#include "core/core.h"
#include "render/Material.h"
#include "render/Mesh.h"
#include "entity/Scene.h"
#include "engine/AssetManager.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace tri {

    FileBrowser::FileBrowser() {
        buttonName = "";
        windowName = "";
        fileTypeId = -1;
        openWindow = false;
        selectedFile = "";
        newFileDirectory = "";
        selectCallback = nullptr;
        fileMenuCallback = nullptr;
        canSelectFiles = false;
        newFileIsDirectory = false;
    }

    void FileBrowser::startup() {
        associations = {
                {".obj", env->reflection->getTypeId<Mesh>()},
                {".scene", env->reflection->getTypeId<Scene>()},
                {".glsl", env->reflection->getTypeId<Shader>()},
                {".png", env->reflection->getTypeId<Texture>()},
                {".jpg", env->reflection->getTypeId<Texture>()},
                {".mat", env->reflection->getTypeId<Material>()},
        };
    }

    int FileBrowser::getFileAssociation(const std::string &extension) {
        auto x = associations.find(extension);
        if(x != associations.end()){
            return x->second;
        }
        return -1;
    }

    void FileBrowser::openBrowseWindow(const std::string &buttonName, const std::string &windowName, int fileTypeId,
        const std::function<void(const std::string &)> &selectCallback) {
        this->buttonName = buttonName;
        this->windowName = windowName;
        this->fileTypeId = fileTypeId;
        this->selectCallback = selectCallback;
        openWindow = true;
    }

    void FileBrowser::browse(const std::function<void(const std::string &, int)> &fileMenuCallback) {
        this->fileMenuCallback = fileMenuCallback;
        for(auto &dir : env->assets->getSearchDirectories()){
            std::string name = std::filesystem::path(dir).parent_path().filename();
            if(ImGui::TreeNode(name.c_str())){
                directory(dir, dir);
                ImGui::TreePop();
            }
        }
    }

    void FileBrowser::update() {
        canSelectFiles = false;
        if(openWindow){
            openWindow = false;
            selectedFile = "";
            ImGui::OpenPopup(windowName.c_str());
        }

        bool open = true;
        if(ImGui::BeginPopupModal(windowName.c_str(), &open)){
            if(!open){
                ImGui::CloseCurrentPopup();
            }

            //directory browsing
            ImGui::BeginChild("child", ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing())), false, ImGuiWindowFlags_HorizontalScrollbar);
            canSelectFiles = true;
            browse();
            canSelectFiles = false;
            ImGui::EndChild();

            ImGui::Separator();
            if(ImGui::Button("Close")){
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            //select button
            if(selectedFile == ""){
                //disable button
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if(ImGui::Button(buttonName.c_str())){
                if(selectCallback){
                    selectCallback(selectedFile);
                }
                ImGui::CloseCurrentPopup();
            }
            if(selectedFile == ""){
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            ImGui::EndPopup();
        }
    }

    void FileBrowser::directory(const std::string &directory, const std::string &searchDirectory) {
        for(auto &entry : std::filesystem::directory_iterator(directory)){
            if(entry.is_directory()){
                if(ImGui::TreeNode(entry.path().filename().c_str())){
                    directoryMenu(entry.path());

                    //new file/directory
                    if(newFileDirectory != "" && newFileDirectory == entry.path().string()){
                        if(inputBuffer.size() == 0){
                            inputBuffer.resize(256);
                        }
                        ImGui::PushID("input");
                        if(ImGui::InputText("", inputBuffer.data(), inputBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)){
                            std::string input = inputBuffer.c_str();
                            if(input.size() != 0) {
                                if(newFileIsDirectory){
                                    std::string path = entry.path().string() + "/" + input;
                                    std::filesystem::create_directory(path);
                                }else{
                                    std::string file = entry.path().string() + "/" + input;
                                    std::ofstream stream(file);
                                }
                            }
                            newFileDirectory = "";
                        }
                        ImGui::PopID();
                    }

                    this->directory(entry.path(), searchDirectory);
                    ImGui::TreePop();
                }
                else{
                    directoryMenu(entry.path());
                }
            }
        }
        for(auto &entry : std::filesystem::directory_iterator(directory)){
            if(entry.is_regular_file()){
                if(canSelectFiles) {
                    //selectable files
                    if (ImGui::Selectable(entry.path().filename().c_str(),
                        selectedFile == entry.path(), ImGuiSelectableFlags_DontClosePopups)) {
                        if (fileTypeId == -1 || getFileAssociation(entry.path().extension()) == fileTypeId) {
                            selectedFile = entry.path();
                        }
                    }
                }else {
                    //file with context menu
                    if(ImGui::TreeNodeEx(entry.path().filename().c_str(), ImGuiTreeNodeFlags_Leaf)){
                        ImGui::TreePop();
                    }
                    if (fileMenuCallback) {
                        fileMenuCallback(entry.path().string(), getFileAssociation(entry.path().extension()));
                    }
                }
            }
        }
    }

    void FileBrowser::directoryMenu(const std::string &directory) {
        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("New File")){
                newFileDirectory = directory;
                newFileIsDirectory = false;
            }
            if(ImGui::MenuItem("New Directory")){
                newFileDirectory = directory;
                newFileIsDirectory = true;
            }
            ImGui::EndPopup();
        }
    }

}