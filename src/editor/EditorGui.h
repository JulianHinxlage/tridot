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
        void textInput(const std::string &label, std::string &text, const std::string &hint = "");

        void update();
        void startup();

    private:
        std::string inputBuffer;
    };

}

