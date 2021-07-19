//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ConsolePanel.h"
#include "Editor.h"
#include <imgui.h>

namespace tridot {

    std::stringstream consoleLog;

    void ConsolePanel::init(){
        Log::Options options;
        options.logLevel = Log::DEBUG;
        options.dateEnabled = false;
        options.timeEnabled = true;
        options.colorEnabled = false;
        Log::addTarget(consoleLog, "console", options);
    }

    void ConsolePanel::update() {
        EditorGui::window("Console", [](){
            std::string str = consoleLog.str();
            std::string line;
            for (char c : str) {
                if (c == '\n') {
                    ImGui::Text("%s", line.c_str());
                    line.clear();
                }
                else {
                    line.push_back(c);
                }
            }
            if (line.size() > 0) {
                ImGui::Text("%s", line.c_str());
                line.clear();
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
        });
    }

}