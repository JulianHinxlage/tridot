//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Engine.h"
#include <GL/glew.h>

tridot::Engine engine;
bool engineInited = false;
bool engineInitedSignal = false;

ecs::Signal<> &getUpdateSignal(){
    static ecs::Signal<> signal;
    return signal;
}

ecs::Signal<> &getInitSignal(){
    static ecs::Signal<> signal;
    return signal;
}

int addUpdateSignalCallback(const std::string &name, const std::function<void()> &callback){
    if(engineInited){
        return engine.onUpdate().add(name, callback);
    }else{
        return getUpdateSignal().add(name, callback);
    }
}

int addInitSignalCallback(const std::string &name, const std::function<void()> &callback){
    int id = -1;
    if(engineInited){
        id = engine.onInit().add(name, callback);
    }else{
        id = getInitSignal().add(name, callback);
    }
    if(engineInitedSignal){
        callback();
    }
    return id;
}

void imguiInit();

namespace tridot {

    Engine::Engine() {
        engineInited = true;
        onUpdateSignal = getUpdateSignal();
        onInitSignal = getInitSignal();
    }

    Engine::~Engine() {
        engineInited = false;
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
        renderer.init(resources.get<Shader>("mesh.glsl"));
        pbRenderer.init(resources.get<Shader>("PBR.glsl"));
        glEnable(GL_DEPTH_TEST);

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

        imguiInit();
        onInitSignal.invoke();
        engineInitedSignal = true;
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

UpdateSignalRegisterer::UpdateSignalRegisterer(const std::string &name, const std::function<void()> &callback){
    this->name = name;
    this->id = addUpdateSignalCallback(name, callback);
}
UpdateSignalRegisterer::~UpdateSignalRegisterer(){
    if(engineInited){
        engine.onUpdate().remove(id);
    }
}

InitSignalRegisterer::InitSignalRegisterer(const std::string &name, const std::function<void()> &callback){
    this->name = name;
    this->id = addInitSignalCallback(name, callback);
}
InitSignalRegisterer::~InitSignalRegisterer(){
    if(engineInited){
        engine.onInit().remove(id);
    }
}
