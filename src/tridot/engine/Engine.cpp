//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Engine.h"
#include "tridot/components/Tag.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/components/ComponentCache.h"
#include "tridot/render/Camera.h"
#include "tridot/engine/Serializer.h"
#include <GL/glew.h>

tridot::Engine engine;

void imguiInit();

bool &isEngineConstructed(){
    static bool value = false;
    return value;
}

bool &isEngineInitialized(){
    static bool value = false;
    return value;
}

bool &isEngineDestructed(){
    static bool value = false;
    return value;
}

ecs::Signal<> &tmpUpdateSignal(){
    static ecs::Signal<> signal;
    return signal;
}

ecs::Signal<> &tmpInitSignal(){
    static ecs::Signal<> signal;
    return signal;
}

namespace tridot {

    Engine::Engine() {
        isEngineConstructed() = true;
        onInitSignal = tmpInitSignal();
        onUpdateSignal = tmpUpdateSignal();
    }

    Engine::~Engine() {
        isEngineDestructed() = true;
    }

    void Engine::init(uint32_t width, uint32_t height, const std::string &title, const std::string &resourceDirectory, bool autoReload) {
        Log::info("Tridot version " TRI_VERSION);

        registerComponent<Tag, uuid, Transform, RenderComponent, PerspectiveCamera, OrthographicCamera, Light, RigidBody, Collider, ComponentCache>();

        window.init(width, height, title);
        input.init();
        time.init();
        resources.addSearchDirectory(resourceDirectory);
        if(resourceDirectory.back() == '/'){
            resources.addSearchDirectory(resourceDirectory + "textures/");
            resources.addSearchDirectory(resourceDirectory + "shaders/");
            resources.addSearchDirectory(resourceDirectory + "models/");
            resources.addSearchDirectory(resourceDirectory + "scenes/");
        }else{
            resources.addSearchDirectory(resourceDirectory + "/textures/");
            resources.addSearchDirectory(resourceDirectory + "/shaders/");
            resources.addSearchDirectory(resourceDirectory + "/models/");
            resources.addSearchDirectory(resourceDirectory + "/scenes/");
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
        onInitSignal.setActiveAll(false);
        isEngineInitialized() = true;
    }

    void Engine::beginScene() {
        onBeginSceneSignal.invoke();
    }

    void Engine::update() {
        onInitSignal.invoke();
        onInitSignal.setActiveAll(false);
        onUpdateSignal.invoke();
    }

    void Engine::run() {
        while(window.isOpen()){
            update();
        }
    }

    void Engine::endScene() {
        onEndSceneSignal.invoke();
    }

    void Engine::shutdown() {
        onShutdownSignal.invoke();
    }

    ecs::SignalRef<> Engine::onInit(){
        if(isEngineConstructed()){
            return onInitSignal.ref();
        }else{
            return tmpInitSignal().ref();
        }
    }

    ecs::SignalRef<> Engine::onBeginScene(){
        return onBeginSceneSignal.ref();
    }

    ecs::SignalRef<> Engine::onUpdate(){
        if(isEngineConstructed()){
            return onUpdateSignal.ref();
        }else{
            return tmpUpdateSignal().ref();
        }
    }

    ecs::SignalRef<> Engine::onEndScene(){
        return onEndSceneSignal.ref();
    }

    ecs::SignalRef<> Engine::onShutdown(){
        return onShutdownSignal.ref();
    }

}

tridot::impl::UpdateSignalRegisterer::UpdateSignalRegisterer(const std::string &name, const std::function<void()> &callback){
    this->id = engine.onUpdate().add(name, callback);
}
tridot::impl::UpdateSignalRegisterer::~UpdateSignalRegisterer(){
    if(!isEngineDestructed()){
        engine.onUpdate().remove(id);
    }
}

tridot::impl::InitSignalRegisterer::InitSignalRegisterer(const std::string &name, const std::function<void()> &callback){
    this->id = engine.onInit().add(name, callback);
}
tridot::impl::InitSignalRegisterer::~InitSignalRegisterer(){
    if(!isEngineDestructed()){
        engine.onInit().remove(id);
    }
}
