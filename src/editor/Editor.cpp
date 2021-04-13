//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "tridot/engine/Engine.h"
#include <imgui.h>
#include <fstream>

namespace tridot {

    ecs::EntityId Editor::selectedEntity = -1;
    ecs::EntityId Editor::cameraId = -1;
    glm::vec2 Editor::viewportSize = {0, 0};
    std::map<std::string, bool> Editor::flags;
    std::string Editor::currentSceneFile = "";
    uint64_t Editor::propertiesWindowFlags = 0;

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

    TRI_INIT("editor"){
        if(ImGui::GetCurrentContext()) {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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
        engine.onUpdate().order({"imgui end", "window", "imgui begin", "clear", "rendering", "editor", "panels"});
    }

    TRI_UPDATE("editor"){
        if(ImGui::GetCurrentContext() != nullptr) {
            ImGui::DockSpaceOverViewport();

            bool openFilePopup = false;
            bool saveFilePopup = false;

            if (ImGui::BeginMainMenuBar()) {
                if(ImGui::BeginMenu("File")){
                    if(ImGui::MenuItem("Open")){
                        openFilePopup = true;
                    }
                    if(ImGui::MenuItem("Save")){
                        engine.saveScene(Editor::currentSceneFile);
                    }
                    if(ImGui::MenuItem("Save as")){
                        saveFilePopup = true;
                    }
                    ImGui::EndMenu();
                }


                if (ImGui::BeginMenu("Panels")) {
                    for (auto &panel : Editor::flags) {
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
                        if(engine.loadScene(std::string(buffer))){
                            Editor::currentSceneFile = std::string(buffer);
                            Editor::cameraId = -1;
                            Editor::selectedEntity = -1;
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
                        Editor::currentSceneFile = std::string(buffer);
                        engine.saveScene(Editor::currentSceneFile);
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

}
