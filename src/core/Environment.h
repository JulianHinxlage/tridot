//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    class SystemManager;
    class Console;
    class ModuleManager;
    class Reflection;
    class Profiler;
    class SignalManager;
    class ThreadPool;
    class Scene;
    class Input;
    class Time;
    class Window;
    class Renderer;
    class Serializer;
    class AssetManager;
    class Editor;
    class HierarchySystem;
    class Physics;
    class Random;
    class RuntimeMode;
    class RenderPipeline;

    class TRI_API Environment {
    public:
        Environment();

        SystemManager *systems;
        Console *console;
        ModuleManager *modules;
        Reflection *reflection;
        Profiler *profiler;
        SignalManager *signals;
        ThreadPool *threads;
        Scene *scene;
        Input *input;
        Time *time;
        Window *window;
        Renderer *renderer;
        Serializer *serializer;
        AssetManager *assets;
        Editor *editor;
        HierarchySystem *hierarchies;
        Physics *physics;
        Random* random;
        RuntimeMode* runtime;
        RenderPipeline* pipeline;

        static Environment* startup();
        static void shutdown();
    };

}

extern TRI_API tri::Environment* env;
