//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/Environment.h"
#include "TypeGui.h"
#include "FileBrowser.h"

namespace tri {

    class EditorGui {
    public:
        TypeGui type;
        FileBrowser file;

        void dragDropSource(int typeId, const std::string &file);
        std::string dragDropTarget(int typeId);

        void update();
        void startup();
    };

}

