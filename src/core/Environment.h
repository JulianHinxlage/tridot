//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    class TRI_API Environment {
    public:
        Environment();

        class SystemManager *systems;
        class Console *console;
        class ModuleManager *modules;
        class Reflection *reflection;
        class Profiler *profiler;
        class SignalManager *signals;
        class ThreadPool *threads;
        class Scene *scene;
        class Input *input;
        class Time *time;
        class Window *window;
        class Renderer *renderer;
        class Serializer *serializer;
        class AssetManager *assets;
        class Editor *editor;
        class HierarchySystem *hierarchies;
        class Physics *physics;
        class Random* random;
        class RuntimeMode* runtime;
        class RenderPipeline* renderPipeline;
        class RenderThread* renderThread;
        class JobSystem* jobSystem;
        class RenderSettings* renderSettings;

        static Environment* startup();
        static void shutdown();
    };

}

extern TRI_API tri::Environment* env;
extern TRI_API tri::Environment* environment;
