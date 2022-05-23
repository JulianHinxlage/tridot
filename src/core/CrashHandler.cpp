//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "CrashHandler.h"
#include "config.h"
#include "Reflection.h"
#include "Console.h"
#include "ModuleManager.h"
#include "EventManager.h"
#include "MainLoop.h"
#include "Profiler.h"

#if TRI_WINDOWS
#include <windows.h>
#endif
#include <csetjmp>

namespace tri {

    TRI_SYSTEM(CrashHandler);

    CrashHandler* crashHandler;
    thread_local void* CrashHandler::recoveryPoint = new jmp_buf();

#if TRI_WINDOWS
    LONG handlerFunction(_EXCEPTION_POINTERS *info) {
        TRI_PROFILE("crash");
        std::string file = ModuleManager::getModuleNameByAddress(info->ExceptionRecord->ExceptionAddress);
        env->console->fatal("crash in module \"%s\"", file.c_str());
        if (crashHandler->enableCrashRecovery) {
            if (crashHandler->unloadModuleOnCrash) {
                env->moduleManager->unloadModule(file, false);
            }
            longjmp(*(jmp_buf*)CrashHandler::recoveryPoint, 0);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
#endif

    void CrashHandler::init() {
        crashHandler = this;
#if TRI_WINDOWS
        SetUnhandledExceptionFilter(&handlerFunction);
#else
        const int signals[] = { SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGSYS };
        for (int i = 0; i < 6; ++i)
        {
            ::signal(signals[i], &handlerFunction);
        }
#endif
    }

    void CrashHandler::shutdown() {
        
    }

}