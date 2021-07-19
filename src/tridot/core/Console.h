//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <fstream>

namespace tridot {

    enum LogLevel{
        NONE,
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL,
        OFF,
    };

    class Console {
    public:
        class Options{
        public:
            LogLevel level;
            bool date;
            bool time;
            bool color;

            Options(LogLevel level = TRACE, bool date = true, bool time = true, bool color = true)
                : level(level), date(date), time(time), color(color) {}
        };
        Options options;

        void log(LogLevel level, const std::string &format, ...);
        void log(LogLevel level, const std::string &format, va_list args);
        void trace(const std::string &format, ...);
        void debug(const std::string &format, ...);
        void info(const std::string &format, ...);
        void warning(const std::string &format, ...);
        void error(const std::string &format, ...);
        void fatal(const std::string &format, ...);

        void addLogFile(const std::string &file, Options options, bool append = false);
        void addLogCallback(Options options, const std::function<void(LogLevel, const std::string &)> &callback);

        void addCommand(const std::string &command, const std::function<void(const std::vector<std::string> &)> &callback);
        void addCommand(const std::string &command, const std::function<void()> &callback);
        void removeCommand(const std::string &command);
        void executeCommand(const std::string &command);

        template<typename T>
        void setVariable(const std::string &name, T *variable){
            size_t type = typeid(T).hash_code();
            variables[name] = { std::shared_ptr<T>(variable, [](T *ptr){}), type};
        }

        template<typename T>
        void setVariable(const std::string &name, T value){
            if(T *var = getVariable<T>(name)){
                *var = value;
            }
        }

        template<typename T>
        T *getVariable(const std::string &name){
            size_t type = typeid(T).hash_code();
            if(!variables.contains(name)){
                variables[name] = { std::make_shared<T>(), type };
            }
            Variable &var = variables[name];
            if(var.type == type){
                return (T*)var.value.get();
            }else{
                return nullptr;
            }
        }

    private:
        class Variable{
        public:
            std::shared_ptr<void> value;
            size_t type;
        };

        class LogFile{
        public:
            std::ofstream stream;
            Options options;
        };

        class LogCallback{
        public:
            std::function<void(LogLevel, const std::string &)> callback;
            Options options;
        };

        std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> commands;
        std::unordered_map<std::string, Variable> variables;
        std::vector<LogFile> logFiles;
        std::vector<LogCallback> logCallbacks;
    };

}

