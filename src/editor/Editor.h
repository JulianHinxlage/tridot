//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/core/Environment.h"
#include "tridot/engine/Scene.h"
#include "tridot/render/Camera.h"
#include "SelectionContext.h"
#include "Viewport.h"
#include "PropertiesPanel.h"
#include "EntitiesPanel.h"
#include "ResourcePanel.h"
#include "ConsolePanel.h"
#include "ResourceBrowser.h"
#include "ProfilerPanel.h"
#include "Undo.h"
#include <glm/glm.hpp>
#include <map>

namespace tridot {

    class Editor{
    public:
        SelectionContext selection;
        Viewport viewport;
        EntitiesPanel entities;
        PropertiesPanel properties;
        ResourcePanel resource;
        ConsolePanel console;
        ResourceBrowser resourceBrowser;
        ProfilerPanel profiler;
        Undo undo;

        EntityId cameraId;
        std::map<std::string, bool> flags;
        uint64_t propertiesWindowFlags;
        bool runtime;
        Scene runtimeSceneBuffer;
        std::vector<std::string> editModeCallbackBlacklist;

        void init();
        void update();
        void updateMenuBar();

        void enableRuntime();
        void disableRuntime(bool restoreScene = true);
        void deactivateCallbackInEditMode(const std::string &callbackName);

        bool &getFlag(const std::string &name);
        void loadFlags();
        void saveFlags();
    };

}

