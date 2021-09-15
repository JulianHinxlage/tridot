//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Console.h"
#include "util/StrUtil.h"
#include "Environment.h"
#include "ThreadPool.h"
#include "SignalManager.h"
#include <iostream>

#if TRI_WINDOWS
#include <conio.h>
static char getCharacter() {
    return _getch();
}
#else
#include <termios.h>
#include <unistd.h>
static char getCharacter() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}
#endif

namespace tri {

    namespace impl {
        void assertLog(const std::string& message) {
            env->console->fatal(message);
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

    const char *getLogLevelName(LogLevel level){
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

    const char *Console::getLogLevelName(LogLevel level) {
        return tri::getLogLevelName(level);
    }

    static void logToStream(std::ostream &stream, Console::Options &options, const std::string &suffix, LogLevel level, const std::string &message) {
        if(level >= options.level && level > NONE && level < OFF){
            char date[32] = "";
            char time[32] = "";

            int newLineIndent = 11;

            if(options.date || options.time) {
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

                if (options.time) {
                    newLineIndent += 15;
                    int pos = (int)strftime(time, 32, "[%H:%M:%S", &timeInfo);
                    snprintf(time + pos, 32 - pos, ".%03d] ", (int)msec);
                }
                if (options.date) {
                    newLineIndent += 13;
                    strftime(date, 32, "[%d.%m.%Y] ", &timeInfo);
                }
            }

            if(options.color){
                stream << date << time << "[\033[" << getLogLevelAnsiColor(level) << "m" << getLogLevelName(level) << "\033[0m] " << getLogLevelSpacing(level) << " ";
            }else{
                stream << date << time << "[" << getLogLevelName(level) << "]" << getLogLevelSpacing(level) << " ";
            }
            for(char c : message){
                stream << c;
                if(c == '\n'){
                    for(int i = 0; i < newLineIndent; i++){
                        stream << ' ';
                    }
                }
            }
            if(!suffix.empty()){
                stream << suffix;
            }
            stream.flush();
        }
    }

    Console::Console() {
        executedCommandIndex = 0;
        inputCursorIndex = 0;
        inputThreadId = -1;
        isPostStartup = false;
    }

    void Console::log(LogLevel level, const std::string &message) {
        static std::mutex mutex;
        mutex.lock();

#if !TRI_DISTRIBUTION
        std::cout << "\r";
        for (int i = 0; i < inputLine.size(); i++) {
            std::cout << " ";
        }
        std::cout << "\r";
        logToStream(std::cout, options, "\n", level, message);
        std::cout << inputLine;
        std::cout.flush();
#endif
        
        for(auto &logFile : logFiles) {
            if (logFile.stream.is_open()) {
                logToStream(logFile.stream, logFile.options, "\n", level, message);
            }
        }
        for(auto &logCallback : logCallbacks) {
            if(logCallback.callback){
                std::ostringstream stream;
                logToStream(stream, logCallback.options, "", level, message);
                logCallback.callback(level, stream.str());
            }
        }
        mutex.unlock();
    }

    void Console::startup(){
        env->signals->postStartup.addCallback("Console", [&](){
            isPostStartup = true;
            for(auto &command : delayedCommands){
                executeCommand(command);
            }
            delayedCommands.clear();
        });

#if !TRI_DISTRIBUTION
        inputThreadId = env->threads->addThread([this]() {
            while (true) {
                char c = getCharacter();
                if (c == 13 || c == '\n') { //enter
                    std::cout << "\r";
                    std::cout << inputLine << "\n";
                    std::string command = inputLine;
                    inputLine.clear();
                    inputCursorIndex = 0;
                    executeCommand(command);
                    if (executedCommands.size() == 0 || command != executedCommands[executedCommands.size() - 1]) {
                        executedCommands.push_back(command);
                    }
                    executedCommandIndex = (int)executedCommands.size();
                }
                else if (c == 127 || c == '\b') { //backspace
                    if (!inputLine.empty() && inputCursorIndex > 0) {
                        std::string line = inputLine;
                        line.erase(line.begin() + inputCursorIndex - 1);
                        inputCursorIndex--;
                        changeInputLine(line);
                    }
                }
                else if (c == '\t') { //tab

                    std::vector<std::string> candidates;

                    for (auto& c : commands) {
                        if (StrUtil::isSubstring(c.first, inputLine) == 2) {
                            candidates.push_back(c.first);
                        }
                    }
                    for (auto& v : variables) {
                        if (StrUtil::isSubstring(v.first, inputLine) == 2) {
                            candidates.push_back(v.first);
                        }
                    }

                    if (candidates.size() > 0) {
                        if (candidates.size() == 1) {
                            inputCursorIndex = (int)candidates[0].size();
                            changeInputLine(candidates[0]);
                        }
                        else {
                            std::sort(candidates.begin(), candidates.end());
                            std::string last = candidates[0];
                            int match = last.size();
                            for (auto& c : candidates) {
                                match = std::min(match, StrUtil::match(c, last));
                                last = c;
                            }
                            info("commands: ", StrUtil::join(candidates, ", "));
                            inputCursorIndex = match;
                            changeInputLine(last.substr(0, match));
                        }
                    }

                }
                else if ((int)c == -32) { //escape character
                    c = getCharacter();

                    if (c == 72) { //arrow up
                        executedCommandIndex--;
                        if (executedCommandIndex < 0) {
                            executedCommandIndex = 0;
                        }
                        if (executedCommandIndex >= 0 && executedCommandIndex < executedCommands.size()) {
                            inputCursorIndex = executedCommands[executedCommandIndex].size();
                            changeInputLine(executedCommands[executedCommandIndex]);
                        }
                        else {
                            inputCursorIndex = 0;
                            changeInputLine("");
                        }
                    }else if (c == 80) { //arrow down
                        executedCommandIndex++;
                        if (executedCommandIndex > executedCommands.size()) {
                            executedCommandIndex = (int)executedCommands.size();
                        }
                        if (executedCommandIndex >= 0 && executedCommandIndex < executedCommands.size()) {
                            inputCursorIndex = executedCommands[executedCommandIndex].size();
                            changeInputLine(executedCommands[executedCommandIndex]);
                        }
                        else {
                            inputCursorIndex = 0;
                            changeInputLine("");
                        }
                    }
                    else if (c == 75) { //arrow left
                        inputCursorIndex--;
                        if (inputCursorIndex < 0) {
                            inputCursorIndex = 0;
                        }
                        changeInputLine(inputLine);
                    }
                    else if (c == 77) { //arrow right
                        inputCursorIndex++;
                        if (inputCursorIndex > inputLine.size()) {
                            inputCursorIndex = inputLine.size();
                        }
                        changeInputLine(inputLine);
                    }
                }
                else {
                    inputLine.insert(inputLine.begin() + inputCursorIndex, c);
                    inputCursorIndex++;
                    changeInputLine(inputLine);
                }
            }
        });
#endif
    }

    void Console::shutdown() {
        env->threads->terminateThread(inputThreadId);
        env->signals->postStartup.removeCallback("Console");
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

    void Console::addCommand(const std::string &command, const std::function<void(const std::vector<std::string> &)> &callback, bool postStartupCommand) {
        commands[command] = {callback, postStartupCommand};
    }

    void Console::addCommand(const std::string &command, const std::function<void()> &callback, bool postStartupCommand) {
        addCommand(command, [callback](const std::vector<std::string> &args){
            if(callback){
                callback();
            }
        }, postStartupCommand);
    }

    void Console::removeCommand(const std::string &command) {
        commands.erase(command);
    }

    void Console::executeCommand(const std::string &command) {
        std::vector<std::string> parts;
        int i = 0;
        for (auto& str : StrUtil::split(command, "\"", true)) {
            if (i & 1) {
                parts.push_back(str);
            }
            else {
                for (auto& str2 : StrUtil::split(str, " ", false)) {
                    parts.push_back(str2);
                }
            }
            i++;
        }

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
                        info(variable.first.c_str(), " = ", value);
                    }

                    if (type == typeid(float).hash_code()) {
                        float &value = *(float *) ptr;
                        try {
                            if(parts.size() > index) {
                                value = (float)stod(parts[index]);
                            }
                        } catch (...) {}
                        info(variable.first.c_str(), " = ", value);
                    }

                    if (type == typeid(double).hash_code()) {
                        double &value = *(double *) ptr;
                        try {
                            if(parts.size() > index) {
                                value = stod(parts[index]);
                            }
                        } catch (...) {}
                        info(variable.first.c_str(), " = ", value);
                    }

                    if (type == typeid(bool).hash_code()) {
                        bool& value = *(bool*)ptr;
                        try {
                            if (parts.size() > index) {
                                value = stoi(parts[index]);
                            }
                        }
                        catch (...) {}
                        info(variable.first.c_str(), " = ", value);
                    }

                    if (type == typeid(std::string).hash_code()) {
                        std::string &value = *(std::string*) ptr;
                        try {
                            if(parts.size() > index) {
                                value = parts[index];
                            }
                        } catch (...) {}
                        info(variable.first.c_str(), " = ", value.c_str());
                    }

                    return;
                }
            }
            for(auto &com : commands){
                if(com.first == parts[0]){
                    if(com.second.callback){
                        if(com.second.postStartupCommand && !isPostStartup){
                            delayedCommands.push_back(command);
                            return;
                        }else{
                            trace("executing command: ", command.c_str());
                            com.second.callback(parts);
                            return;
                        }
                    }
                }
            }

            info("command not found: ", parts[0].c_str());
        }
    }

    void Console::loadConfigFile(const std::string& file){
        info("loading config file ", file);
        std::string config = StrUtil::readFile(file);
        for (auto& line : StrUtil::split(config, "\n", false)) {
            if (line.size() > 0 && line[0] != '#') {
                executeCommand(line);
            }
        }
    }

    void Console::changeInputLine(const std::string& newInput){
        std::cout << "\r";
        for (int i = 0; i < inputLine.size(); i++) {
            std::cout << " ";
        }
        std::cout << "\r";
        inputLine = newInput;
        std::cout << inputLine;
        std::cout << "\r";
        std::cout << inputLine.substr(0, inputCursorIndex);
        std::cout.flush();
    }

}
