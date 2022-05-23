//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"

namespace tri {

    class CrashHandler : public System {
    public:
        bool enableCrashRecovery = true;
        bool unloadModuleOnCrash = true;
        static thread_local void* recoveryPoint;
        virtual void init() override;
        virtual void shutdown() override;
    };

}
