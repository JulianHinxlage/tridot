//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "tridot/engine/Input.h"
#include "tridot/render/Window.h"
#include "tridot/components/Tag.h"
#include <imgui.h>
#include <fstream>

namespace tridot {

    TRI_INIT_CALLBACK("editor"){
        env->editor = env->systems->addSystem<Editor>();
    }

    bool &Editor::getFlag(const std::string &name){
        auto open = flags.find(name);
        if(open == flags.end()){
            flags[name] = false;
            return flags[name];
        }else{
            return open->second;
        }
    }

    void Editor::loadFlags(){
        std::ifstream s("flags.ini");
        if(!s.is_open()) {
            s.open(env->resources->searchFile("default/flags.ini"));
        }
        if(s.is_open()){
            std::string line;
            while(std::getline(s, line, '\n')){
                auto pos = line.find('=');
                bool value = (bool)std::stoi(line.substr(pos+1));
                std::string key = line.substr(0, pos);
                flags[key] = value;
            }
        }
    }

    void Editor::saveFlags(){
        std::ofstream s("flags.ini");
        for(auto &panel : flags){
            s.write(panel.first.c_str(), panel.first.size());
            s.write("=", 1);
            s.write(std::to_string((int)panel.second).c_str(), 1);
            s.write("\n", 1);
        }
    }

    void Editor::enableRuntime() {
        runtimeSceneBuffer.copy(*env->scene);
        env->events->update.setActiveAll(true);
        runtime = true;
        env->events->sceneBegin.invoke();
        undo.enabled = false;
        env->console->debug("runtime enabled");
    }

    void Editor::disableRuntime(bool restoreScene) {
        env->events->sceneEnd.invoke();
        if(restoreScene){
            PerspectiveCamera cameraBuffer;
            Transform cameraTransformBuffer;
            if(env->scene->hasAll<PerspectiveCamera, Transform>(env->editor->cameraId)){
                cameraBuffer = env->scene->get<PerspectiveCamera>(env->editor->cameraId);
                cameraTransformBuffer = env->scene->get<Transform>(env->editor->cameraId);
            }
            env->scene->copy(runtimeSceneBuffer);
            if(env->scene->hasAll<PerspectiveCamera, Transform>(env->editor->cameraId)){
                env->scene->get<PerspectiveCamera>(env->editor->cameraId) = cameraBuffer;
                env->scene->get<Transform>(env->editor->cameraId) = cameraTransformBuffer;
            }
        }
        runtimeSceneBuffer.clear();
        env->events->update.setActiveAll(false);
        env->events->update.setActiveCallback("rendering", true);
        env->events->update.setActiveCallback("panels", true);
        env->events->update.setActiveCallback("window", true);
        env->events->update.setActiveCallback("imgui begin", true);
        env->events->update.setActiveCallback("imgui end", true);
        env->events->update.setActiveCallback("input", true);
        env->events->update.setActiveCallback("time", true);
        env->events->update.setActiveCallback("editor", true);
        env->events->update.setActiveCallback("clear", true);
        env->events->update.setActiveCallback("resources", true);
        env->events->update.setActiveCallback("transform", true);
        env->events->update.setActiveCallback("profiler", true);
        env->events->update.setActiveCallback("post processing", true);
        env->events->update.setActiveCallback("skybox", true);
        runtime = false;
        undo.enabled = true;
        env->console->debug("runtime disabled");
    }

    void Editor::init() {
        if(ImGui::GetCurrentContext()) {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            if(!std::filesystem::exists("editor.ini")){
                std::string path = env->resources->searchFile("default/editor.ini");
                if(path != ""){
                    std::filesystem::copy(path, "editor.ini");
                }
            }
            ImGui::GetIO().IniFilename = "editor.ini";

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.3));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.4));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.5));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1, 1, 1, 0.2));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1, 1, 1, 0.3));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1, 1, 1, 0.4));
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1, 1, 1, 0.1));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1, 1, 1, 0.25));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(1, 1, 1, 0.40));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(1, 1, 1, 0.1));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(1, 1, 1, 0.25));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.35));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1, 1, 1, 0.45));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1, 1, 1, 0.55));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.35));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1, 1, 1, 0.35));
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 1, 1, 0.45));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.116, 0.125, 0.133, 1));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.177, 0.177, 0.177, 1));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.238, 0.238, 0.238, 1));
            ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(0.0, 0.32, 1.0, 1));
        }

        viewport.init();
        console.init();
        resourceBrowser.init();
        profiler.init();

        env->events->update.addCallback("editor", [](){
            env->editor->update();
        });
        env->events->update.callbackOrder({"post processing", "editor", "panels"});
        env->editor->disableRuntime(false);
    }

    void Editor::update() {
        TRI_PROFILE("editor")

        if(ImGui::GetCurrentContext() != nullptr) {
            ImGui::DockSpaceOverViewport();
            updateMenuBar();
        }

        {
            TRI_PROFILE("editor/viewport")
            viewport.update();
        }
        {
            TRI_PROFILE("editor/entities")
            entities.update();
        }
        {
            TRI_PROFILE("editor/properties")
            properties.update();
        }
        {
            TRI_PROFILE("editor/resource panles")
            resource.update();
        }
        {
            TRI_PROFILE("editor/console")
            console.update();
        }
        {
            TRI_PROFILE("editor/resource browser")
            resourceBrowser.update();
        }
        {
            TRI_PROFILE("editor/profiler")
            profiler.update();
        }

        if(!runtime) {
            if (env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL)) {
                if (env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT)) {
                    if (env->input->pressed('Y')) {
                        undo.redoAction();
                    }
                } else {
                    if (env->input->pressed('Y')) {
                        undo.undoAction();
                    }
                }
            }
        }
    }

    void Editor::updateMenuBar() {
        bool openFilePopup = false;
        bool saveFilePopup = false;

        if (ImGui::BeginMainMenuBar()) {
            if(ImGui::BeginMenu("File")){
                if(ImGui::MenuItem("Open")){
                    openFilePopup = true;
                }
                if(ImGui::MenuItem("Save")){
                    if(runtime){
                        disableRuntime();
                    }
                    env->scene->save();
                }
                if(ImGui::MenuItem("Save as")){
                    saveFilePopup = true;
                }
                if(ImGui::MenuItem("Close")){
                    env->resources->setup<Scene>("").setPreLoad(nullptr).get();
                }
                if(ImGui::MenuItem("Exit")){
                    env->window->close();
                }
                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("View")) {
                for (auto &panel : env->editor->flags) {
                    if (ImGui::MenuItem(panel.first.c_str(), nullptr, panel.second)) {
                        panel.second = !panel.second;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        //Open File
        if(openFilePopup){
            ImGui::OpenPopup("Open File");
        }
        {
            bool popupOpen = true;
            if (ImGui::BeginPopupModal("Open File", &popupOpen)){
                if (!popupOpen) {
                    ImGui::CloseCurrentPopup();
                }
                static char buffer[256];
                ImGui::InputText("File", buffer, sizeof(buffer));
                if (ImGui::Button("Open")) {
                    if(runtime){
                        disableRuntime();
                    }
                    env->editor->undo.clearActions();

                    std::string file(buffer);
                    if(env->resources->searchFile(file) != ""){
                        file = env->resources->searchFile(file);
                        env->resources->get<Scene>(file);
                        env->editor->cameraId = -1;
                        env->editor->selection.unselect();
                    }else{
                        env->console->warning("file ", file, " not found");
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if(ImGui::Button("Cancel")){
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }


        //Save File
        if(saveFilePopup){
            ImGui::OpenPopup("Save File");
        }
        {
            bool popupOpen = true;
            if (ImGui::BeginPopupModal("Save File", &popupOpen)){
                if (!popupOpen) {
                    ImGui::CloseCurrentPopup();
                }
                static char buffer[256];
                ImGui::InputText("File", buffer, sizeof(buffer));
                if (ImGui::Button("Save")) {
                    if(runtime){
                        disableRuntime();
                    }
                    env->scene->save(std::string(buffer));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if(ImGui::Button("Cancel")){
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

}
