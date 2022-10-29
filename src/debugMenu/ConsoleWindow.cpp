//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "window/UIManager.h"
#include "core/Reflection.h"
#include "core/Environment.h"
#include "window/Window.h"
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace tri {

	class ConsoleWindow : public UIWindow {
	public:

		class LogEntry {
		public:
			std::string msg;
			LogLevel level;
			std::string time;
			std::string date;
			std::string source;
		};
		std::vector<LogEntry> entries;
		std::vector<std::string> history;
		int historyIndex = -1;
		std::string command;
		LogLevel level = LogLevel::DEBUG;

		void init() override {
			env->systemManager->addSystem<UIManager>();
			env->uiManager->addWindow<ConsoleWindow>("Console", "Debug");
			Console::LogTarget target;
			target.callback = [&](const std::string& msg, LogLevel level, const std::string& time, const std::string& date, const std::string& source) {
				LogEntry entry;
				entry.msg = msg;
				entry.level = level;
				entry.time = time;
				entry.date = date;
				entry.source = source;
				entries.push_back(entry);
			};
			env->console->addLogTarget(target);
		}

		void tick() override {
			if (env->window && env->window->inFrame()) {
				if (ImGui::Begin("Console", &active)) {

					const char *levels[] = { "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };
					ImGui::Combo("Level", (int*)&level, levels, 6);
					ImGui::Separator();

					if (ImGui::BeginChild("child", ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing())), false, ImGuiWindowFlags_HorizontalScrollbar)) {
						for (auto& entry : entries) {
							if (entry.level >= level) {
								ImGui::Text("[%s] [%s] %s", entry.time.c_str(), env->console->getLevelName(entry.level), entry.msg.c_str());
							}
						}
						if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
							ImGui::SetScrollHereY(1.0f);
						}
					}
					ImGui::EndChild();

					ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
					if (ImGui::InputTextWithHint("", "command", &command, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, [](ImGuiInputTextCallbackData *data) {
						ConsoleWindow* win = (ConsoleWindow*)data->UserData;

						if (data->EventFlag & ImGuiInputTextFlags_CallbackCompletion) {
							win->command = env->console->autoComplete(win->command);
							data->BufDirty = true;
							data->BufSize = win->command.capacity();
							data->BufTextLen = win->command.size();
							data->Buf = win->command.data();
							data->CursorPos = win->command.size();
						}
						else if (data->EventFlag & ImGuiInputTextFlags_CallbackHistory) {
							if (win->historyIndex == -1) {
								win->historyIndex = win->history.size() - 1;
							}
							else {
								if (data->EventKey == ImGuiKey_UpArrow) {
									win->historyIndex--;
								}
								else if (data->EventKey == ImGuiKey_DownArrow) {
									win->historyIndex++;
								}
							}

							if (win->historyIndex < 0) {
								win->historyIndex = 0;
							}
							if (win->historyIndex >= win->history.size()) {
								win->historyIndex = win->history.size() - 1;
							}
							win->command = win->history[win->historyIndex];

							data->BufDirty = true;
							data->BufSize = win->command.capacity();
							data->BufTextLen = win->command.size();
							data->Buf = win->command.data();
							data->CursorPos = win->command.size();
						}

						return 0;
					}, this)) {
						env->console->executeCommand(command);
						history.push_back(command);
						historyIndex = -1;
						command.clear();
					}

				}
				ImGui::End();
			}
		}
	};

	TRI_SYSTEM(ConsoleWindow);

}