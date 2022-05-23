//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"
#include "Profiler.h"

namespace tri {

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

    class Console : public System {
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

        class Message{
        public:
            LogLevel level;
            std::string message;
        };

        Console();
        virtual void startup() override;
        virtual void shutdown() override;

        void log(LogLevel level, const std::string &message);

        template<typename... T>
        void log(LogLevel level, const T ... t){
            TRI_PROFILE("log");
            std::stringstream stream;
            ((stream << t), ...);
            log(level, stream.str());
        }

        template<typename... T>
        void trace(const T ... t){
            log(TRACE, t...);
        }
        template<typename... T>
        void debug(const T ... t){
            log(DEBUG, t...);
        }
        template<typename... T>
        void info(const T ... t){
            log(INFO, t...);
        }
        template<typename... T>
        void warning(const T ... t){
            log(WARNING, t...);
        }
        template<typename... T>
        void error(const T ... t){
            log(ERROR, t...);
        }
        template<typename... T>
        void fatal(const T ... t){
            log(FATAL, t...);
        }

        void addLogFile(const std::string &file, Options options, bool append = false);
        void addLogCallback(Options options, const std::function<void(LogLevel, const std::string &)> &callback);
        const char *getLogLevelName(LogLevel level);

        void addCommand(const std::string &command, const std::function<void(const std::vector<std::string> &)> &callback, bool postStartupCommand = false);
        void addCommand(const std::string &command, const std::function<void()> &callback, bool postStartupCommand = false);
        void removeCommand(const std::string &command);
        void executeCommand(const std::string &command);
        std::string autoCompleteCommand(const std::string &command);

        void loadConfigFile(const std::string& file);

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
            if(variables.find(name) == variables.end()){
                variables[name] = { std::make_shared<T>(), type };
            }
            Variable &var = variables[name];
            if(var.type == type){
                return (T*)var.value.get();
            }else{
                return nullptr;
            }
        }

        const std::vector<Message> &getMessages(){
            return messages;
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

        class Command{
        public:
            std::function<void(const std::vector<std::string> &)> callback;
            bool postStartupCommand;
        };

        void changeInputLine(const std::string& newInput);

        std::unordered_map<std::string, Command> commands;
        std::unordered_map<std::string, Variable> variables;
        std::vector<LogFile> logFiles;
        std::vector<LogCallback> logCallbacks;
        std::vector<std::string> delayedCommands;
        std::vector<Message> messages;
        bool isPostStartup;
        int inputThreadId;
        std::string inputLine;
        int inputCursorIndex;
        std::vector<std::string> executedCommands;
        int executedCommandIndex;
    };

}

