//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "tridot/engine/Engine.h"
#include <imgui.h>

using namespace tridot;

std::stringstream consoleLog;

TRI_INIT("panels"){
    Log::Options options;
    options.logLevel = Log::DEBUG;
    options.dateEnabled = false;
    options.timeEnabled = true;
    options.colorEnabled = false;
    Log::addTarget(consoleLog, "console", options);
}

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr){
        bool &open = Editor::getFlag("Console");
        if(open){
            if(ImGui::Begin("Console", &open, ImGuiWindowFlags_HorizontalScrollbar)){
                ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
                std::string str = consoleLog.str();
                std::string line;
                for(char c : str){
                    if(c == '\n'){
                        ImGui::Text("%s", line.c_str());
                        line.clear();
                    }else{
                        line.push_back(c);
                    }
                }
                if(line.size() > 0){
                    ImGui::Text("%s", line.c_str());
                    line.clear();
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()){
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::End();
        }
    }
}
