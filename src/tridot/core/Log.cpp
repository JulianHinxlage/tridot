//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Log.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <time.h>

namespace tridot{

    Log::Options Log::options;

    class Target{
    public:
        std::shared_ptr<std::ofstream> stream;
        std::string file;
        Log::Options options;
    };

    std::vector<Target> logTargets;

    void Log::addTarget(const std::string &file, Log::Options options) {
        std::shared_ptr<std::ofstream> s = std::make_shared<std::ofstream>();
        s->open(file, std::ofstream::app);
        logTargets.push_back({s, file, options});
    }

    void Log::removeTarget(const std::string &file) {
        for(int i = 0; i < logTargets.size(); i++){
            if(logTargets[i].file == file){
                logTargets.erase(logTargets.begin() + i);
                i--;
            }
        }
    }

    const char* logGetLevelName(Log::Level level){
        switch (level){
            case Log::Level::TRACE:
                return "TRACE";
            case Log::Level::DEBUG:
                return "DEBUG";
            case Log::Level::INFO:
                return "INFO";
            case Log::Level::WARNING:
                return "WARNING";
            case Log::Level::ERROR:
                return "ERROR";
            case Log::Level::CRITICAL:
                return "CRITICAL";
            default:
                return "NONE";
        }
    }

    const char* logGetLevelColor(Log::Level level){
        switch (level){
            case Log::Level::TRACE:
                return "0;90";
            case Log::Level::DEBUG:
                return "0;90";
            case Log::Level::INFO:
                return "0";
            case Log::Level::WARNING:
                return "1;33";
            case Log::Level::ERROR:
                return "1;31";
            case Log::Level::CRITICAL:
                return "1;35";
            default:
                return "0";
        }
    }

    std::string logGetTime(){
        std::stringstream stream;

        time_t rawTime;
        time(&rawTime);
        struct tm *timeInfo;
        timeInfo = localtime(&rawTime);

        if (timeInfo->tm_hour < 10) {
            stream << "0";
        }
        stream << timeInfo->tm_hour << ":";

        if (timeInfo->tm_min < 10) {
            stream << "0";
        }
        stream << timeInfo->tm_min << ":";

        if (timeInfo->tm_sec < 10) {
            stream << "0";
        }
        stream << timeInfo->tm_sec;

        return stream.str();
    }

    std::string logGetDate(){
        std::stringstream stream;

        time_t rawTime;
        time(&rawTime);
        struct tm *timeInfo;
        timeInfo = localtime(&rawTime);

        if (timeInfo->tm_mday < 10) {
            stream << "0";
        }
        stream << timeInfo->tm_mday << ".";

        if (timeInfo->tm_mon + 1 < 10) {
            stream << "0";
        }
        stream << timeInfo->tm_mon + 1 << ".";

        int year = timeInfo->tm_year + 1900;
        stream << year;

        return stream.str();
    }

    void logStream(tridot::Log::Level level, const std::string &message, Log::Options &options, std::ostream &stream) {
        if(level >= options.logLevel && level >= Log::TRACE && level < Log::OFF){
            std::stringstream s;
            std::string levelName = logGetLevelName(level);
            int maxLevelNameLength = 8;
            int newLineIndent = maxLevelNameLength + 3;

            if(options.dateEnabled){
                s << "[" << logGetDate() << "] ";
                newLineIndent += 13;
            }
            if(options.timeEnabled){
                s << "[" << logGetTime() << "] ";
                newLineIndent += 11;
            }
            if(options.colorEnabled){
                s << "[\033[" << logGetLevelColor(level) << "m" << levelName << "\033[0m] ";
            }else{
                s << "[" << levelName << "] ";
            }

            for(int i = 0; i < maxLevelNameLength - (int)levelName.size();i++){
                s << " ";
            }
            for(char c : message){
                if(c == '\n'){
                    s << "\n";
                    for(int i = 0; i < newLineIndent;i++){
                        s << " ";
                    }
                }else{
                    s << c;
                }
            }

            s << std::endl;
            stream << s.str();
            stream.flush();
        }
    }

    void Log::log(tridot::Log::Level level, const std::string &message) {
        logStream(level, message, options, std::cout);
        for(auto &target : logTargets){
            if(target.stream){
                if(target.stream->is_open()){
                    logStream(level, message, target.options, *target.stream);
                }
            }
        }
    }

}

