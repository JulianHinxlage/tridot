//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/Input.h"
#include <imgui/imgui.h>

namespace tri {

    class ConsoleWindow;
    ConsoleWindow *consoleWindow = nullptr;

    class ConsoleWindow : public EditorWindow {
    public:
        LogLevel level;
        std::string inputBuffer;
        std::vector<std::string> commands;
        int commandIndex;

        void startup() override {
            name = "Console";
            level = DEBUG;
            commandIndex = 0;
            consoleWindow = this;
        }

        void update() override {
            if(ImGui::BeginCombo("Level", env->console->getLogLevelName(level))){
                for(int i = LogLevel::TRACE; i < LogLevel::OFF; i++){
                    if(ImGui::Selectable(env->console->getLogLevelName((LogLevel)i), i == (int)level)){
                        level = (LogLevel)i;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();
            ImGui::BeginChild("console child", ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing())), false, ImGuiWindowFlags_HorizontalScrollbar);
            for(auto &msg : env->console->getMessages()){
                if(msg.level >= level){
                    ImGui::Text("%s", msg.message.c_str());
                }
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
            if (ImGui::InputTextWithHint("", "command", inputBuffer.data(), inputBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways, [](ImGuiInputTextCallbackData* data){
                std::string command = consoleWindow->inputBuffer.c_str();
                bool change = false;
                if(env->input->pressed(Input::KEY_TAB)){
                    command = env->console->autoCompleteCommand(command);
                    change = true;
                }
                if(env->input->pressed(Input::KEY_UP)){
                    consoleWindow->commandIndex--;
                    if(consoleWindow->commandIndex < 0){
                        consoleWindow->commandIndex = 0;
                    }
                    if(consoleWindow->commandIndex < consoleWindow->commands.size()){
                        command = consoleWindow->commands[consoleWindow->commandIndex];
                        change = true;
                    }else{
                        command = "";
                        change = true;
                    }
                }
                if(env->input->pressed(Input::KEY_DOWN)){
                    consoleWindow->commandIndex++;
                    if(consoleWindow->commandIndex > consoleWindow->commands.size()){
                        consoleWindow->commandIndex = consoleWindow->commands.size();
                    }
                    if(consoleWindow->commandIndex < consoleWindow->commands.size()){
                        command = consoleWindow->commands[consoleWindow->commandIndex];
                        change = true;
                    }else{
                        command = "";
                        change = true;
                    }
                }
                if(change) {
                    for (int i = 0; i < command.size() + 1; i++) {
                        consoleWindow->inputBuffer[i] = command[i];
                        data->Buf[i] = command[i];
                    }
                    data->CursorPos = command.size();
                    data->BufTextLen = command.size();
                    data->BufDirty = true;
                }
                return 0;
            })){
                env->console->executeCommand(inputBuffer.c_str());
                if(commands.size() == 0 || commands.back() != std::string(inputBuffer.c_str())){
                    commands.push_back(inputBuffer.c_str());
                }
                commandIndex = commands.size();
                inputBuffer = "";
                ImGui::SetKeyboardFocusHere(-1);
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
    };

    TRI_STARTUP_CALLBACK("") {
        editor->addWindow(new ConsoleWindow);
    }

}
