//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/core/Environment.h"

namespace tridot {

    class EventListener {
    public:
        EventListener();
        ~EventListener();

        virtual void onInit();
        virtual void onUpdate();
        virtual void onShutdown();
        virtual void onSceneBegin();
        virtual void onSceneEnd();

    private:
        int initCallbackId;
        int updateCallbackId;
        int shutdownCallbackId;
        int sceneBeginCallbackId;
        int sceneEndCallbackId;
    };

}

