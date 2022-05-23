//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RuntimeMode.h"

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(RuntimeMode, env->runtime);

    void RuntimeMode::startup() {
        env->signals->postStartup.addCallback("RuntimeMode", [&]() {
            if (env->editor) {
                setMode(EDIT);
            }
            else {
                setMode(RUNTIME);
            }
        });
        env->signals->preShutdown.addCallback("RuntimeMode", [&]() {
            setMode(SHUTDOWN);
        });
        env->signals->update.onAddCallback([&](const std::string &name) {
            if (mode == EDIT || mode == PAUSE) {
                env->signals->update.setActiveCallback(name, false);
            }
            else {
                env->signals->update.setActiveCallback(name, true);
            }
            if (mode == PAUSE) {
                for (auto& c : callbacks[EDIT]) {
                    if (c.name == name) {
                        env->signals->update.setActiveCallback(c.name, c.active);
                    }
                }
            }
            for (auto& c : callbacks[mode]) {
                if (c.name == name) {
                    env->signals->update.setActiveCallback(c.name, c.active);
                }
            }
        });
    }

    RuntimeMode::Mode RuntimeMode::getMode() {
        return mode;
    }

    RuntimeMode::Mode RuntimeMode::getPreviousMode() {
        return previousMode;
    }
    
    void RuntimeMode::setMode(RuntimeMode::Mode mode) {
        if (mode != this->mode) {
            TRI_PROFILE("setRuntimeMode");
            if (mode == RUNTIME) {
                TRI_PROFILE_INFO("runtime", 7);
            }else if (mode == EDIT) {
                TRI_PROFILE_INFO("edit", 4);
            }
            else if (mode == PAUSE) {
                TRI_PROFILE_INFO("pause", 5);
            }
            previousMode = this->mode;
            this->mode = mode;

            if (mode == EDIT || mode == PAUSE) {
                env->signals->update.setActiveAll(false);
            }
            else {
                env->signals->update.setActiveAll(true);
            }
            if (mode == PAUSE) {
                for (auto& c : callbacks[EDIT]) {
                    env->signals->update.setActiveCallback(c.name, c.active);
                }
            }
            for (auto& c : callbacks[mode]) {
                env->signals->update.setActiveCallback(c.name, c.active);
            }

            env->signals->runtimeModeChanged.invoke();
        }
    }

    void RuntimeMode::setActive(const std::string& callback, bool active, Mode mode) {
        for (auto& c : callbacks[mode]) {
            if (c.name == callback) {
                c.active = active;
                if (this->mode == mode) {
                    env->signals->update.setActiveCallback(callback, active);
                }
                return;
            }
        }
        callbacks[mode].push_back({ callback, active });
        if (this->mode == mode) {
            env->signals->update.setActiveCallback(callback, active);
        }
    }

    bool RuntimeMode::getActive(const std::string& callback, Mode mode) {
        bool active = false;
        if (mode == EDIT || mode == PAUSE) {
            active = false;
        }
        else {
            active = true;
        }
        if (mode == PAUSE) {
            for (auto& c : callbacks[EDIT]) {
                if (c.name == callback) {
                    active = c.active;
                    break;
                }
            }
        }
        for (auto& c : callbacks[mode]) {
            if (c.name == callback) {
                active = c.active;
                break;
            }
        }
        return active;
    }


}
