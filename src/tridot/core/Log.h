//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_LOG_H
#define TRIDOT_LOG_H

#include <sstream>

namespace tridot {

    template<typename... T>
    static std::string str(T... t){
        std::stringstream s;
        ((s << t) , ...);
        return s.str();
    }

    class Log{
    public:
        enum Level {
            TRACE = 0,
            DEBUG = 1,
            INFO = 2,
            WARNING = 3,
            ERROR = 4,
            CRITICAL = 5,
            OFF = 6
        };

        struct Options{
            Level logLevel = DEBUG;
            bool dateEnabled = true;
            bool timeEnabled = true;
            bool colorEnabled = true;
        };

        static Options options;
        static void addTarget(const std::string &file, Options options);
        static void removeTarget(const std::string &file);

        static void log(Level level, const std::string &message);

        template<typename... T>
        static void log(Level level, T... t){
            log(level, str(t...));
        }
        template<typename... T>
        static void trace(T... t){
            log(TRACE, t...);
        }
        template<typename... T>
        static void debug(T... t){
            log(DEBUG, t...);
        }
        template<typename... T>
        static void info(T... t){
            log(INFO, t...);
        }
        template<typename... T>
        static void warning(T... t){
            log(WARNING, t...);
        }
        template<typename... T>
        static void error(T... t){
            log(ERROR, t...);
        }
        template<typename... T>
        static void critical(T... t){
            log(CRITICAL, t...);
        }
    };

}

#endif //TRIDOT_LOG_H
