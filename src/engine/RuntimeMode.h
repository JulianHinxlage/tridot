//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/core.h"

namespace tri {

    class RuntimeMode : public System {
    public:
        enum Mode {
            STARTUP,
            RUNTIME,
            PAUSE,
            EDIT,
            SHUTDOWN,
        };

        virtual void startup() override;
        Mode getMode();
        Mode getPreviousMode();
        void setMode(Mode mode);
        void setActive(const std::string &callback, bool active, Mode mode);
    
    private:
        Mode mode = STARTUP;
        Mode previousMode = STARTUP;

        class Callback {
        public:
            std::string name;
            bool active;
        };
        std::unordered_map<Mode, std::vector<Callback>> callbacks;
    };

}

