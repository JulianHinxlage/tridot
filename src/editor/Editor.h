//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/core.h"
#include "EditorWindow.h"
#include "SelectionContext.h"
#include "EditorGui.h"
#include "EntityOperations.h"
#include "Gizmos.h"
#include "UndoSystem.h"
#include "PropertiesWindow.h"
#include "ViewportWindow.h"

namespace tri {

    class Editor : public System {
    public:
        RuntimeMode mode;
        SelectionContext selectionContext;
        EditorGui gui;
        EntityOperations entityOperations;
        Gizmos gizmos;
        UndoSystem undo;
        PropertiesWindow properties;
        ViewportWindow viewport;

        void startup() override;
        void update() override;
        void shutdown() override;
        void addWindow(EditorWindow* window);
        void updateMenuBar();
        void setMode(RuntimeMode mode);

    private:
        std::vector<EditorWindow*> windows;
        bool updated;
        Ref<Scene> sceneBuffer;
    };

}


