//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EventListener.h"

namespace tridot {

    EventListener::EventListener() {
        initCallbackId = env->events->init.addCallback([this](){onInit();});
        updateCallbackId = env->events->update.addCallback([this](){onUpdate();});
        shutdownCallbackId = env->events->shutdown.addCallback([this](){onShutdown();});
        sceneBeginCallbackId = env->events->sceneBegin.addCallback([this](){onSceneBegin();});
        sceneEndCallbackId = env->events->sceneEnd.addCallback([this](){onSceneEnd();});
    }

    EventListener::~EventListener() {
        env->events->init.removeCallback(initCallbackId);
        env->events->update.removeCallback(updateCallbackId);
        env->events->shutdown.removeCallback(shutdownCallbackId);
        env->events->sceneBegin.removeCallback(sceneBeginCallbackId);
        env->events->sceneEnd.removeCallback(sceneEndCallbackId);
    }

    void EventListener::onInit() {

    }

    void EventListener::onUpdate() {

    }

    void EventListener::onShutdown() {

    }

    void EventListener::onSceneBegin() {

    }

    void EventListener::onSceneEnd() {

    }

}
