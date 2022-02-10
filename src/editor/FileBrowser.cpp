//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "FileBrowser.h"
#include "core/core.h"
#include "render/Material.h"
#include "render/Mesh.h"
#include "entity/Scene.h"
#include "engine/AssetManager.h"
#include "entity/Prefab.h"
#include "core/util/StrUtil.h"
#include "engine/Time.h"
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
        updatingTree = false;
    }

    void FileBrowser::startup() {
        associations = {
                {".obj", env->reflection->getTypeId<Mesh>()},
                {".scene", env->reflection->getTypeId<Scene>()},
                {".glsl", env->reflection->getTypeId<Shader>()},
                {".png", env->reflection->getTypeId<Texture>()},
                {".jpg", env->reflection->getTypeId<Texture>()},
                {".mat", env->reflection->getTypeId<Material>()},
                {".prefab", env->reflection->getTypeId<Prefab>()},
                {".so", env->reflection->getTypeId<Module>()},
                {".dll", env->reflection->getTypeId<Module>()},
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
    
    void FileBrowser::browse(const std::function<void(const std::string &, int)> &fileMenuCallback) {
        this->fileMenuCallback = fileMenuCallback;
        mutex.lock();
        browse(rootNode);
        mutex.unlock();

        if ((env->time->frameTicks(5) || rootNode.directories.size() != env->assets->getSearchDirectories().size()) && !updatingTree) {
            updatingTree = true;
            env->threads->addTask([&]() {
                TRI_PROFILE("updateDirectoryTree");
                updateRootNode = rootNode;
                for (auto& dir : env->assets->getSearchDirectories()) {
                    bool exists = false;
                    for (auto& node : updateRootNode.directories) {
                        if (dir == node.path) {
                            node.update();
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        updateRootNode.directories.emplace_back(dir);
                        updateRootNode.directories.back().update();
                    }
                }
                mutex.lock();
                rootNode = updateRootNode;
                updateRootNode.directories.clear();
                mutex.unlock();
                updatingTree = false;
            });
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

    void FileBrowser::newFileBox(const std::string &directory) {
        if(newFileDirectory != "" && newFileDirectory == directory){
            if(inputBuffer.size() == 0){
                inputBuffer.resize(256);
            }
            ImGui::PushID("input");
            if(ImGui::InputText("", inputBuffer.data(), inputBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)){
                std::string input = inputBuffer.c_str();
                if(input.size() != 0) {
                    if(newFileIsDirectory){
                        std::string path = directory + "/" + input;
                        std::filesystem::create_directory(path);
                    }else{
                        std::string file = directory + "/" + input;
                        std::ofstream stream(file);
                    }
                }
                newFileDirectory = "";
            }
            ImGui::PopID();
        }
    }

    FileBrowser::DirectoryNode::DirectoryNode(const std::string& path) {
        auto p = std::filesystem::path(path);
        isFile = !std::filesystem::is_directory(p);
        this->path = StrUtil::replace(p.string(), "\\", "/");
        if (path.size() > 0 && path.back() == '/') {
            this->name = StrUtil::replace(p.parent_path().filename().string(), "\\", "/");
        }
        else {
            this->name = StrUtil::replace(p.filename().string(), "\\", "/");
        }
        this->extension = StrUtil::replace(p.extension().string(), "\\", "/");
        shouldUpdate = true;
    }

    void FileBrowser::DirectoryNode::update() {
        if (!isFile && shouldUpdate) {
            files.clear();
            auto oldDirectories = directories;
            directories.clear();

            for (auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) {

                    bool shouldAdd = true;
                    for (auto& dir : oldDirectories) {
                        if (dir.path == entry.path().string()) {
                            if (!dir.shouldUpdate) {
                                directories.push_back(dir);
                                shouldAdd = false;
                                break;
                            }
                        }
                    }
                    if (shouldAdd) {
                        directories.emplace_back(entry.path().string());
                        directories.back().update();
                    }
                }
                else {
                    files.emplace_back(entry.path().string());
                    files.back().update();
                }
            }
        }
    }

    void FileBrowser::browse(FileBrowser::DirectoryNode &node) {
        for (auto& n : node.directories) {
            if (ImGui::TreeNode(n.name.c_str())) {
                n.shouldUpdate = true;
                directoryMenu(n.path);
                newFileBox(n.path);
                if (std::filesystem::exists(n.path)) {
                    browse(n);
                }
                ImGui::TreePop();
            }
            else {
                n.shouldUpdate = false;
                directoryMenu(n.path);
            }
        }
        for (auto& n : node.files) {
            if (canSelectFiles) {
                //selectable files
                if (ImGui::Selectable(n.name.c_str(),
                    selectedFile == n.path, ImGuiSelectableFlags_DontClosePopups)) {
                    if (fileTypeId == -1 || getFileAssociation(n.extension) == fileTypeId) {
                        selectedFile = n.path;
                    }
                }
            }
            else {
                //file with context menu
                if (ImGui::TreeNodeEx(n.name.c_str(), ImGuiTreeNodeFlags_Leaf)) {
                    ImGui::TreePop();
                }
                if (fileMenuCallback) {
                    fileMenuCallback(n.path, getFileAssociation(n.extension));
                }
            }
        }
    }

}
