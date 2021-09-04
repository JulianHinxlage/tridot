//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ConsolePanel.h"
#include "Editor.h"
#include <imgui.h>

namespace tridot {

    void ConsolePanel::init(){
        logLevelFilter = DEBUG;
        env->console->addLogCallback(Console::Options(TRACE, false, true, false), [this](LogLevel level, const std::string &message){
            messages.push_back({level, message + "\n"});
        });
    }

    void ConsolePanel::update() {
        EditorGui::window("Console", [this](){
            if(ImGui::BeginCombo("level", env->console->getLogLevelName(logLevelFilter))) {
                for(int i = TRACE; i <= FATAL; i++){
                    if(ImGui::Selectable(env->console->getLogLevelName((LogLevel)i), logLevelFilter == (LogLevel)i)){
                        logLevelFilter = (LogLevel)i;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Separator();
            ImGui::BeginChild("console child", ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing())), false, ImGuiWindowFlags_HorizontalScrollbar);
            std::string line;
            for(auto &message : messages) {
                if(message.level >= logLevelFilter) {
                    for (char c : message.message) {
                        if (c == '\n') {
                            ImGui::Text("%s", line.c_str());
                            line.clear();
                        } else {
                            line.push_back(c);
                        }
                    }
                }
            }
            if (line.size() > 0) {
                ImGui::Text("%s", line.c_str());
                line.clear();
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
            ImGui::Separator();



            if(inputBuffer.size() == 0){
                inputBuffer.resize(256);
            }
            ImGui::PushID("console input");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
            if (ImGui::InputTextWithHint("", "command", inputBuffer.data(), inputBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)){
                env->console->executeCommand(inputBuffer.c_str());
                inputBuffer = "";
                ImGui::SetKeyboardFocusHere(-1);
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        });
    }

}