//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"
#include "Reflection.h"
#include "Environment.h"
#include "util/StrUtil.h"
#include <stdarg.h>

namespace tri {

	enum class LogLevel {
		TRACE,
		DEBUG,
		INFO,
		WARNING,
		ERROR,
		FATAL,
	};

	class CVar {
	public:
		template<typename T>
		T* getPtr() {
			if (classId == Reflection::getClassId<T>()) {
				return (T*)ptr;
			}
			else {
				return nullptr;
			}
		}

		template<typename T>
		T get() {
			if (classId == Reflection::getClassId<T>()) {
				return *(T*)ptr;
			}
			else {
				T t;
				std::stringstream s;
				s << toString();
				s >> t;
				return t;
			}
		}

		template<typename T>
		void set(const T& value) {
			if (classId == Reflection::getClassId<T>()) {
				*(T*)ptr = value;
			}
			else {
				std::stringstream s;
				s << value;
				fromString(s.str());
			}
		}

		template<typename T>
		bool isType() {
			return classId == Reflection::getClassId<T>();
		}

		virtual std::string toString() {
			return "";
		}

		virtual void fromString(const std::string& str) {}

	protected:
		friend class Console;
		int classId;
		std::string name;
		void* ptr;
	};

	class Console : public System {
	public:
		class LogTarget {
		public:
			LogLevel level = LogLevel::TRACE;
			std::set<std::string> sourceFilter;

			bool includeDate = true;
			bool includeTime = true;
			bool includeSource = true;
			bool includeLevel = true;

			std::string file = "";
			std::function<void(const std::string& msg, LogLevel level, const std::string& time, const std::string& date, const std::string& source)> callback = nullptr;
			std::ostream* stream = nullptr;
		};
		
		void logMsg(const std::string& msg, LogLevel level = LogLevel::INFO, const std::string& source = "System");
		void addLogTarget(LogTarget target);
		const char* getLevelName(LogLevel level);

		template<typename... Args>
		std::string format(const char* fmt, Args... args) {

			//bool hasStrings = ((std::is_same_v<Args, std::string>) || ...);
			//if (hasStrings) {
			//	return format(fmt,
			//		(std::is_same_v<Args, std::string>) ? args.c_str() : args)...
			//	);
			//}

			int size = std::snprintf(nullptr, 0, fmt, args...) + 1;
			if (size > 0) {
				std::string msg(size, '\0');
				std::snprintf(msg.data(), size, fmt, args...);
				return msg;
			}
			return "";
		}

		template<typename... Args>
		void log(LogLevel level, const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), level, source);
		}

		template<typename... Args>
		void trace(const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::TRACE);
		}
		template<typename... Args>
		void debug(const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::DEBUG);
		}
		template<typename... Args>
		void info(const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::INFO);
		}
		template<typename... Args>
		void warning(const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::WARNING);
		}
		template<typename... Args>
		void error(const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::ERROR);
		}
		template<typename... Args>
		void fatal(const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::FATAL);
		}

		template<typename... Args>
		void traceSource(const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::TRACE, source);
		}
		template<typename... Args>
		void debugSource(const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::DEBUG, source);
		}
		template<typename... Args>
		void infoSource(const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::INFO, source);
		}
		template<typename... Args>
		void warningSource(const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::WARNING, source);
		}
		template<typename... Args>
		void errorSource(const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::ERROR, source);
		}
		template<typename... Args>
		void fatalSource(const std::string& source, const char* fmt, Args... args) {
			logMsg(format(fmt, args...), LogLevel::FATAL, source);
		}

		CVar* getCVar(const std::string& name);

		template<typename T>
		T getCVarValue(const std::string &name, T fallbackValue = T()) {
			if (auto *cvar = getCVar(name)) {
				return cvar->get<T>();
			}
			else {
				return fallbackValue;
			}
		}

		template<typename T>
		void setCVarValue(const std::string& name, T value) {
			if (auto *cvar = getCVar(name)) {
				cvar->set<T>(value);
			}
		}

		template<typename T>
		CVar* addCVar(const std::string& name, T initialValue = T()) {
			if (cvars.find(name) == cvars.end()) {
				auto cvar = std::make_shared<CVarT<T>>();
				cvar->classId = Reflection::getClassId<T>();
				cvar->name = name;
				cvar->store = initialValue;
				cvar->ptr = &cvar->store;
				cvars[name] = cvar;
				return (CVar*)cvar.get();
			}
			else {
				return cvars[name].get();
			}
		}

		template<typename T>
		CVar* addCVar(const std::string& name, T *ptr) {
			if (cvars.find(name) == cvars.end()) {
				auto cvar = std::make_shared<CVarT<T>>();
				cvar->classId = Reflection::getClassId<T>();
				cvar->name = name;
				cvar->ptr = ptr;
				cvars[name] = cvar;
				return (CVar*)cvar.get();
			}
			else {
				return cvars[name].get();
			}
		}

		void removeCVar(const std::string& name);
		void addCommand(const std::string& name, const std::function<void(const std::vector<std::string>& args)>& callback);
		void removeCommand(const std::string& name);
		void executeCommand(const std::string& command);
		std::string autoComplete(const std::string& command, bool printOptions = true);

	private:
		class Command {
		public:
			std::string name;
			std::function<void(const std::vector<std::string>&)> callback;
		};

		template<typename T>
		class CVarT : public CVar {
		public:
			T store;

			virtual std::string toString() {
				std::stringstream s;
				s << *(T*)ptr;
				return s.str();
			}

			virtual void fromString(const std::string& str) {
				std::stringstream s;
				if (str == "true") {
					s << true;
				}
				else if (str == "false") {
					s << false;
				}
				else {
					s << str;
				}
				s >> *(T*)ptr;
			}
		};

		template<>
		class CVarT<std::string> : public CVar {
		public:
			std::string store;

			virtual std::string toString() {
				return *(std::string*)ptr;
			}

			virtual void fromString(const std::string& str) {
				*(std::string*)ptr = str;
			}
		};

		std::vector<std::shared_ptr<LogTarget>> targets;
		std::unordered_map<std::string, std::shared_ptr<CVar>> cvars;
		std::unordered_map<std::string, std::shared_ptr<Command>> commands;
	};

}