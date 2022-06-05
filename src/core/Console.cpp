//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Console.h"
#include "config.h"
#include "Environment.h"
#include "Profiler.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(Console, env->console);
	tri::impl::GlobalInitializationCallback consoleInit([]() { env->console = env->systemManager->addSystem<Console>(); });

	const char* Console::getLevelName(LogLevel level) {
		switch (level)
		{
		case tri::LogLevel::TRACE:
			return "Trace";
		case tri::LogLevel::DEBUG:
			return "Debug";
		case tri::LogLevel::INFO:
			return "Info";
		case tri::LogLevel::WARNING:
			return "Warning";
		case tri::LogLevel::ERROR:
			return "Error";
		case tri::LogLevel::FATAL:
			return "Fatal";
		default:
			return "None";
		}
	}

	void Console::logMsg(const std::string& msg, LogLevel level, const std::string& source) {
		if (this == nullptr) {
			return;
		}

		TRI_PROFILE_FUNC();

		static std::mutex mutex;
		std::unique_lock<std::mutex> lock(mutex);

		char date[32] = "";
		char time[32] = "";
		auto now = std::chrono::system_clock::now().time_since_epoch();
		auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
		auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(now - sec).count();

		time_t timeVal = sec.count();
		struct tm timeInfo;
#if TRI_WINDOWS
		localtime_s(&timeInfo, &timeVal);
#else
		timeInfo = *localtime(&timeVal);
#endif
		int pos = (int)strftime(time, 32, "%H:%M:%S", &timeInfo);
		snprintf(time + pos, 32 - pos, ".%03d", (int)msec);
		strftime(date, 32, "%d.%m.%Y", &timeInfo);
		
		for (auto& target : targets) {
			if (target) {
				if (level >= target->level) {
					if (target->sourceFilter.empty() || target->sourceFilter.find(source) != target->sourceFilter.end()) {
						if (target->callback) {
							target->callback(msg, level, time, date, source);
						}
						if (target->stream) {
							if (target->includeDate) {
								(*target->stream) << "[" << date << "] ";
							}
							if (target->includeTime) {
								(*target->stream) << "[" << time << "] ";
							}
							if (target->includeSource) {
								(*target->stream) << "[" << source << "] ";
							}
							if (target->includeLevel) {
								(*target->stream) << "[" << getLevelName(level) << "] ";
							}
							(*target->stream) << msg.c_str() << std::endl;
						}
					}
				}
			}
		}
	}

	void Console::addLogTarget(LogTarget target) {
		if (!target.file.empty()) {
			target.stream = new std::ofstream(target.file);
		}
		targets.push_back(std::make_shared<LogTarget>(target));
	}

	CVar* Console::getCVar(const std::string& name) {
		auto entry = cvars.find(name);
		if (entry != cvars.end()) {
			return entry->second.get();
		}
		return nullptr;
	}

	void Console::removeCVar(const std::string& name) {
		cvars.erase(name);
	}

	void Console::addCommand(const std::string& name, const std::function<void(const std::vector<std::string>& args)>& callback) {
		auto command = std::make_shared<Command>();
		command->name = name;
		command->callback = callback,
			commands[name] = command;
	}

	void Console::removeCommand(const std::string& name) {
		commands.erase(name);
	}

	void Console::executeCommand(const std::string& command) {
		auto parts1 = StrUtil::split(command, "\"");
		std::vector<std::string> parts;
		for (int i = 0; i < parts1.size(); i++) {
			if (i % 2 == 0) {
				for (auto& p : StrUtil::split(parts1[i], " ", false)) {
					parts.push_back(p);
				}
			}
			else {
				parts.push_back(parts1[i]);
			}
		}

		if (parts.size() > 0) {
			std::string name = parts[0];
			parts.erase(parts.begin());

			if (commands.find(name) != commands.end()) {
				if (commands[name]->callback) {
					commands[name]->callback(parts);
				}
			}
			else if (cvars.find(name) != cvars.end()) {
				if (parts.size() == 0) {
					info("%s = %s", name.c_str(), cvars[name]->toString().c_str());
				}
				else {
					std::string value = parts[0];
					if (parts[0] == "=" && parts.size() > 1) {
						value = parts[1];
					}
					cvars[name]->fromString(value);
					info("%s = %s", name.c_str(), cvars[name]->toString().c_str());
				}
			}
			else {
				info("Command or CVar \"%s\" not found", name.c_str());
			}
		}
	}

	std::string Console::autoComplete(const std::string& command, bool printOptions) {
		std::vector<std::string> options;
		for (auto& com : commands) {
			int match = StrUtil::match(command, com.first);
			if (match >= command.size()) {
				options.push_back(com.first);
			}
		}
		for (auto& cvar : cvars) {
			int match = StrUtil::match(command, cvar.first);
			if (match >= command.size()) {
				options.push_back(cvar.first);
			}
		}

		int minMatch = -1;
		for (int i = 0; i < options.size(); i++) {
			int match = StrUtil::match(options[i], options[0]);
			if (minMatch == -1 || match < minMatch) {
				minMatch = match;
			}
			if (printOptions) {
				info("%s", options[i].c_str());
			}
		}
		if (minMatch != -1) {
			return options[0].substr(0, minMatch);
		}
		return command;
	}

}
