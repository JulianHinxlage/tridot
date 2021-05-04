//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ENGINE_H
#define TRIDOT_ENGINE_H

#include "tridot/ecs/Registry.h"
#include "Scene.h"
#include "Time.h"
#include "Input.h"
#include "Physics.h"
#include "ResourceManager.h"
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

        Engine();
        ~Engine();
        void init(uint32_t width, uint32_t height, const std::string &title, const std::string &resourceDirectory, bool autoReload = false);
        void beginScene();
        void update();
        void run();
        void endScene();
        void shutdown();

        ecs::SignalRef<> onInit();
        ecs::SignalRef<> onBeginScene();
        ecs::SignalRef<> onUpdate();
        ecs::SignalRef<> onEndScene();
        ecs::SignalRef<> onShutdown();

    private:
        ecs::Signal<> onInitSignal;
        ecs::Signal<> onBeginSceneSignal;
        ecs::Signal<> onUpdateSignal;
        ecs::Signal<> onEndSceneSignal;
        ecs::Signal<> onShutdownSignal;
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
namespace{ tridot::impl::UpdateSignalRegisterer ECS_UNIQUE_NAME(___tri_global___)(name, &func);}\
static void func()
#define TRI_UPDATE(name) TRI_UPDATE_2(name, ECS_UNIQUE_NAME(___tri_update_func___))
#define TRI_INIT_2(name, func) static void func();\
namespace{ tridot::impl::InitSignalRegisterer ECS_UNIQUE_NAME(___tri_global___)(name, &func);}\
static void func()
#define TRI_INIT(name) TRI_INIT_2(name, ECS_UNIQUE_NAME(___tri_init_func___))
#define TRI_COMPONENT(type) namespace{ tridot::impl::ComponentRegisterer<type> ECS_UNIQUE_NAME(___tri_global___); }

#endif //TRIDOT_ENGINE_H
