//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_EDITOR_H
#define TRIDOT_EDITOR_H

#include "tridot/ecs/Registry.h"
#include "tridot/render/Camera.h"
#include "SelectionContext.h"
#include "Viewport.h"
#include "PropertiesPanel.h"
#include "EntitiesPanel.h"
#include "ResourcePanel.h"
#include "ConsolePanel.h"
#include "Undo.h"
#include <glm/glm.hpp>
#include <map>

namespace tridot {

    class Editor {
    public:
        static SelectionContext selection;
        static Viewport viewport;
        static EntitiesPanel entities;
        static PropertiesPanel properties;
        static ResourcePanel resource;
        static ConsolePanel console;
        static Undo undo;

        static ecs::EntityId cameraId;
        static std::map<std::string, bool> flags;
        static std::string currentSceneFile;
        static uint64_t propertiesWindowFlags;
        static bool runtime;
        static ecs::Registry runtimeSceneBuffer;

        static void init();
        static void update();

        static void enableRuntime();
        static void disableRuntime(bool restoreScene = true);

        static bool &getFlag(const std::string &name);
        static void loadFlags();
        static void saveFlags();
    };

}

#endif //TRIDOT_EDITOR_H
