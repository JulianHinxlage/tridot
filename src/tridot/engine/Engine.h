//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_ENGINE_H
#define TRIDOT_ENGINE_H

#include "tridot/ecs/Registry.h"
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

    class Engine : public ecs::Registry {
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
        void update();
        void run();
        void shutdown();

        auto onUpdate(){
            return onUpdateSignal.ref();
        }

        auto onInit(){
            return onInitSignal.ref();
        }

        auto onShutdown(){
            return onShutdownSignal.ref();
        }

    private:
        ecs::Signal<> onUpdateSignal;
        ecs::Signal<> onInitSignal;
        ecs::Signal<> onShutdownSignal;
    };

}

extern TRI_API tridot::Engine engine;

#define TRI_UNIQUE_NAME_3(name, line, number) name##line##number
#define TRI_UNIQUE_NAME_2(name, line, number) TRI_UNIQUE_NAME_3(name, line, number)
#define TRI_UNIQUE_NAME(name) TRI_UNIQUE_NAME_2(name, __LINE__, __COUNTER__)

int addUpdateSignalCallback(const std::string &name, const std::function<void()> &callback);
int addInitSignalCallback(const std::string &name, const std::function<void()> &callback);

class UpdateSignalRegisterer{
public:
    std::string name;
    int id;
    UpdateSignalRegisterer(const std::string &name, const std::function<void()> &callback);
    ~UpdateSignalRegisterer();
};

class InitSignalRegisterer{
public:
    std::string name;
    int id;
    InitSignalRegisterer(const std::string &name, const std::function<void()> &callback);
    ~InitSignalRegisterer();
};

template<typename T>
class ComponentRegisterer{
public:
    int id;
    ComponentRegisterer(){
        id = addInitSignalCallback("", [](){
            engine.registerComponent<T>();
        });
    }
    ~ComponentRegisterer(){
        engine.unregisterComponent<T>();
        engine.onInit().remove(id);
    }
};


#define TRI_UPDATE_2(name, func) static void func();\
namespace{ UpdateSignalRegisterer TRI_UNIQUE_NAME(___tri_global___)(name, &func);}\
static void func()
#define TRI_UPDATE(name) TRI_UPDATE_2(name, TRI_UNIQUE_NAME(___tri_update_func___))
#define TRI_INIT_2(name, func) static void func();\
namespace{ InitSignalRegisterer TRI_UNIQUE_NAME(___tri_global___)(name, &func);}\
static void func()
#define TRI_INIT(name) TRI_INIT_2(name, TRI_UNIQUE_NAME(___tri_init_func___))
#define TRI_COMPONENT(type) namespace{ ComponentRegisterer<type> TRI_UNIQUE_NAME(___tri_global___); }

#endif //TRIDOT_ENGINE_H
