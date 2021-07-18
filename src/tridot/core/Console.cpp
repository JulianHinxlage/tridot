//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Console.h"
#include "Environment.h"
#include "tridot/util/StrUtil.h"
#include <cstdarg>
#include <chrono>
#include <thread>
#include <iostream>

namespace tridot {

    static const char *getLogLevelName(LogLevel level){
        switch (level) {
            case NONE:
                return "NONE";
            case TRACE:
                return "TRACE";
            case DEBUG:
                return "DEBUG";
            case INFO:
                return "INFO";
            case WARNING:
                return "WARNING";
            case ERROR:
                return "ERROR";
            case FATAL:
                return "FATAL";
            default:
                return "NONE";
        }
    }

    static const char *getLogLevelSpacing(LogLevel level){
        switch (level) {
            case NONE:
                return "   ";
            case TRACE:
                return "  ";
            case DEBUG:
                return "  ";
            case INFO:
                return "   ";
            case WARNING:
                return "";
            case ERROR:
                return "  ";
            case FATAL:
                return "  ";
            default:
                return "   ";
        }
    }

    static const char* getLogLevelAnsiColor(LogLevel level){
        switch (level){
            case NONE:
                return "0";
            case TRACE:
                return "0;90";
            case DEBUG:
                return "0;90";
            case INFO:
                return "0";
            case WARNING:
                return "1;33";
            case ERROR:
                return "1;31";
            case FATAL:
                return "1;35";
            default:
                return "0";
        }
    }

    static void logToStream(std::ostream &stream, Console::Options &options, bool newLine, LogLevel level, const std::string &format, va_list args) {
        if(level >= options.level && level > NONE && level < OFF){
            char date[32] = "";
            char time[32] = "";

            int newLineIndent = 10;

            if(options.date || options.time) {
                auto now = std::chrono::system_clock::now().time_since_epoch();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
                auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(now - sec).count();

                timeval timeVal;
                timeVal.tv_sec = sec.count();
                timeVal.tv_usec = msec * 1000;
                struct tm *timeInfo = localtime(&timeVal.tv_sec);

                if (options.time) {
                    newLineIndent += 15;
                    int pos = strftime(time, 32, "[%H:%M:%S", timeInfo);
                    snprintf(time + pos, 32 - pos, ".%03d] ", (int)msec);
                }
                if (options.date) {
                    newLineIndent += 13;
                    strftime(date, 32, "[%d.%m.%Y] ", timeInfo);
                }
            }

            if(options.color){
                stream << date << time << "[\033[" << getLogLevelAnsiColor(level) << "m" << getLogLevelName(level) << "\033[0m] " << getLogLevelSpacing(level);
            }else{
                stream << date << time << "[" << getLogLevelName(level) << "]" << getLogLevelSpacing(level);
            }
            char buffer[1024 * 32];
            int size = vsprintf(buffer, format.c_str(), args);
            for(int i = 0; i < size; i++){
                char c = buffer[i];
                stream << c;
                if(c == '\n'){
                    for(int j = 0; j < newLineIndent; j++){
                        stream << ' ';
                    }
                }
            }
            if(newLine){
                stream << "\n";
            }
            stream.flush();
        }
    }

    void Console::log(LogLevel level, const std::string &format, va_list args) {
        va_list localArgs;
        va_copy(localArgs, args);
        logToStream(std::cout, options, true, level, format, localArgs);
        for(auto &logFile : logFiles) {
            if (logFile.stream.is_open()) {
                va_copy(localArgs, args);
                logToStream(logFile.stream, logFile.options, true, level, format, localArgs);
            }
        }
        for(auto &logCallback : logCallbacks) {
            if(logCallback.callback){
                va_copy(localArgs, args);
                std::ostringstream stream;
                logToStream(stream, logCallback.options, false, level, format, localArgs);
                logCallback.callback(level, stream.str());
            }
        }
    }

    void Console::log(LogLevel level, const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(level, format, args);
    }

    void Console::trace(const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(TRACE, format, args);
    }

    void Console::debug(const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(DEBUG, format, args);
    }

    void Console::info(const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(INFO, format, args);
    }

    void Console::warning(const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(WARNING, format, args);
    }

    void Console::error(const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(ERROR, format, args);
    }

    void Console::fatal(const std::string &format, ...) {
        va_list args;
        va_start(args, format);
        log(FATAL, format, args);
    }

    void Console::addLogFile(const std::string &file, Console::Options options, bool append) {
        logFiles.emplace_back();
        auto &logFile = logFiles.back();
        logFile.options = options;
        logFile.stream.open(file, append ? std::ios::out | std::ios::app : std::ios::out);
    }

    void Console::addLogCallback(Console::Options options, const std::function<void(LogLevel, const std::string &)> &callback) {
        logCallbacks.emplace_back();
        auto &logCallback = logCallbacks.back();
        logCallback.options = options;
        logCallback.callback = callback;
    }

    void Console::addCommand(const std::string &command, const std::function<void(const std::vector<std::string> &)> &callback) {
        commands[command] = callback;
    }

    void Console::addCommand(const std::string &command, const std::function<void()> &callback) {
        addCommand(command, [callback](const std::vector<std::string> &args){
            if(callback){
                callback();
            }
        });
    }

    void Console::removeCommand(const std::string &command) {
        commands.erase(command);
    }

    void Console::executeCommand(const std::string &command) {
        std::vector<std::string> parts = StrUtil::split(command, " ", false);
        if(parts.size() > 0){
            for(auto &variable : variables){
                if(variable.first == parts[0]){

                    int index = 1;
                    if (parts.size() > 1 && parts[1] == "=" && parts.size() > 2) {
                        index = 2;
                    }

                    size_t type = variable.second.type;
                    void *ptr = variable.second.value.get();

                    if (type == typeid(int).hash_code()) {
                        int &value = *(int *) ptr;
                        try {
                            if(parts.size() > index) {
                                value = stoi(parts[index]);
                            }
                        } catch (...) {}
                        info("%s = %i", variable.first.c_str(), value);
                    }

                    if (type == typeid(float).hash_code()) {
                        float &value = *(float *) ptr;
                        try {
                            if(parts.size() > index) {
                                value = stod(parts[index]);
                            }
                        } catch (...) {}
                        info("%s = %f", variable.first.c_str(), value);
                    }

                    if (type == typeid(double).hash_code()) {
                        double &value = *(double *) ptr;
                        try {
                            if(parts.size() > index) {
                                value = stod(parts[index]);
                            }
                        } catch (...) {}
                        info("%s = %f", variable.first.c_str(), value);
                    }

                    if (type == typeid(std::string).hash_code()) {
                        std::string &value = *(std::string*) ptr;
                        try {
                            if(parts.size() > index) {
                                value = parts[index];
                            }
                        } catch (...) {}
                        info("%s = %s", variable.first.c_str(), value.c_str());
                    }

                    return;
                }
            }
            for(auto &com : commands){
                if(com.first == parts[0]){
                    if(com.second){
                        info("executing command: %s", command.c_str());
                        com.second(parts);
                        return;
                    }
                }
            }
        }

        info("command not found: %s", parts[0].c_str());
    }

    std::thread consoleInputThread;
    TRI_INIT_CALLBACK("console"){
        env->console->addCommand("exit", [](){
            env->events->exit.invoke();
        });
        env->console->addCommand("quit", [](){
            env->events->exit.invoke();
        });
        env->events->exit.addCallback([](){
            consoleInputThread.detach();
        });
        consoleInputThread = std::thread([](){
            while(true){
                std::string command;
                std::getline(std::cin, command);
                env->console->executeCommand(command);
            }
        });
    }

}
