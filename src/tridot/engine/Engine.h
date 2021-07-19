//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/entity/Registry.h"
#include "Scene.h"
#include "Time.h"
#include "Input.h"
#include "Physics.h"
#include "ResourceManager.h"
#include "Profiler.h"
#include "tridot/render/MeshRenderer.h"
#include "tridot/render/PBRenderer.h"
#include "tridot/render/Window.h"
#include "tridot/core/Log.h"
#include "tridot/core/config.h"

namespace tridot {

    class Engine : public Scene {
    public:
        Time time;
        Input input;
        Physics physics;
        ResourceManager resources;
        MeshRenderer renderer;
        PBRenderer pbRenderer;
        Window window;
        Profiler profiler;

        Engine();
        ~Engine();
        void init(uint32_t width, uint32_t height, const std::string &title, const std::string &resourceDirectory, bool autoReload = false);
        void beginScene();
        void update();
        void run();
        void endScene();
        void shutdown();

        SignalRef<> onInit();
        SignalRef<> onBeginScene();
        SignalRef<> onUpdate();
        SignalRef<> onEndScene();
        SignalRef<> onShutdown();

        template<typename... Components>
        void registerComponent(){
            ComponentRegister::registerComponent<Components...>();
        }

        template<typename... Components>
        void unregisterComponent(){
            ComponentRegister::unregisterComponent<Components...>();
        }

        void unregisterComponent(int typeId){
            ComponentRegister::unregisterComponent(typeId);
        }

        auto onRegister(){
            return ComponentRegister::onRegister();
        }

        auto onUnregister(){
            return ComponentRegister::onUnregister();
        }

    private:
        Signal<> onInitSignal;
        Signal<> onBeginSceneSignal;
        Signal<> onUpdateSignal;
        Signal<> onEndSceneSignal;
        Signal<> onShutdownSignal;
    };

}

extern TRI_API tridot::Engine engine;

namespace tridot::impl {

    class UpdateSignalRegisterer {
    public:
        int id;
        UpdateSignalRegisterer(const std::string &name, const std::function<void()> &callback);
        ~UpdateSignalRegisterer();
    };

    class InitSignalRegisterer {
    public:
        int id;
        InitSignalRegisterer(const std::string &name, const std::function<void()> &callback);
        ~InitSignalRegisterer();
    };

    template<typename T>
    class ComponentRegisterer {
    public:
        int id;
        ComponentRegisterer() {
            id = engine.onInit().add("", []() {
                engine.registerComponent<T>();
            });
        }
        ~ComponentRegisterer() {
            engine.unregisterComponent<T>();
            engine.onInit().remove(id);
        }
    };
}

#define TRI_UPDATE_2(name, func) static void func();\
namespace{ tridot::impl::UpdateSignalRegisterer TRI_UNIQUE_NAME(___tri_global___)(name, &func);}\
static void func()
#define TRI_UPDATE(name) TRI_UPDATE_2(name, TRI_UNIQUE_NAME(___tri_update_func___))
#define TRI_INIT_2(name, func) static void func();\
namespace{ tridot::impl::InitSignalRegisterer TRI_UNIQUE_NAME(___tri_global___)(name, &func);}\
static void func()
#define TRI_INIT(name) TRI_INIT_2(name, TRI_UNIQUE_NAME(___tri_init_func___))
#define TRI_COMPONENT(type) namespace{ tridot::impl::ComponentRegisterer<type> TRI_UNIQUE_NAME(___tri_global___); }

