//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "tridot/engine/Engine.h"
#include <imgui.h>
#include <fstream>

tridot::Editor editor;

namespace tridot {

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
                editor.flags[key] = value;
            }
        }
    }

    void Editor::saveFlags(){
        std::ofstream s("flags.ini");
        for(auto &panel : editor.flags){
            s.write(panel.first.c_str(), panel.first.size());
            s.write("=", 1);
            s.write(std::to_string((int)panel.second).c_str(), 1);
            s.write("\n", 1);
        }
    }

    TRI_INIT("editor"){
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        engine.onUpdate().order({"window", "clear", "rendering", "editor", "panels"});
    }

    TRI_UPDATE("editor"){
        if(ImGui::GetCurrentContext() != nullptr) {
            ImGui::DockSpaceOverViewport();
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("Panels")) {
                    for (auto &panel : editor.flags) {
                        if (ImGui::MenuItem(panel.first.c_str(), nullptr, panel.second)) {
                            panel.second = !panel.second;
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        }
    }

}
