//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Engine.h"
#include <GL/glew.h>

tridot::Engine engine;

ecs::Signal<> &getUpdateSignal(){
    static ecs::Signal<> signal;
    return signal;
}

ecs::Signal<> &getInitSignal(){
    static ecs::Signal<> signal;
    return signal;
}

bool addUpdateSignalCallback(const std::string &name, const std::function<void()> &callback){
    getUpdateSignal().add(name, callback);
    engine.onUpdate().add(name, callback);
    return true;
}

bool addInitSignalCallback(const std::string &name, const std::function<void()> &callback){
    getInitSignal().add(name, callback);
    engine.onInit().add(name, callback);
    return true;
}

namespace tridot {

    Engine::Engine() {
        onUpdateSignal = getUpdateSignal();
        onInitSignal = getInitSignal();
    }

    void Engine::init(uint32_t width, uint32_t height, const std::string &title, const std::string &resourceDirectory, bool autoReload) {
        Log::info("Tridot version " TRI_VERSION);
        window.init(width, height, title);
        input.init();
        time.init();
        if(resourceDirectory.back() == '/'){
            resources.addSearchDirectory(resourceDirectory + "textures/");
            resources.addSearchDirectory(resourceDirectory + "shaders/");
            resources.addSearchDirectory(resourceDirectory + "models/");
        }else{
            resources.addSearchDirectory(resourceDirectory + "/textures/");
            resources.addSearchDirectory(resourceDirectory + "/shaders/");
            resources.addSearchDirectory(resourceDirectory + "/models/");
        }
        resources.autoReload = autoReload;
        physics.init();
        renderer.init(resources.get<Shader>("meshBase.glsl"));
        glEnable(GL_DEPTH_TEST);

        onInitSignal.invoke();

        onUpdate().add("time", [this](){
            time.update();
        });
        onUpdate().add("input", [this](){
            input.update();
        });
        onUpdate().add("resources", [this](){
            resources.update();
        });
        onUpdate().add("window", [this](){
            glFinish();
            window.update();
        });
    }

    void Engine::update() {
        onUpdateSignal.invoke();
    }

    void Engine::run() {
        while(window.isOpen()){
            update();
        }
    }

}