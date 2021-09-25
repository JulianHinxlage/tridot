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

namespace tri {

    class Editor : public System {
    public:
        bool runtimeMode;
        SelectionContext selectionContext;
        EditorGui gui;
        EntityOperations entityOperations;
        Gizmos gizmos;

        void startup() override;
        void update() override;
        void shutdown() override;
        void addWindow(EditorWindow* window);
        void updateMenuBar();

    private:
        std::vector<EditorWindow*> windows;
        bool updated;
    };

}


