//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/Environment.h"
#include "EditorElement.h"
#include "TypeGui.h"
#include "FileBrowser.h"

namespace tri {

    class EditorGui : public EditorElement {
    public:
        TypeGui typeGui;
        FileBrowser fileGui;

        void dragDropSource(int typeId, const std::string &file);
        std::string dragDropTarget(int typeId);
        void textInput(const std::string &label, std::string &text, const std::string &hint = "");

        void update() override;
        void startup() override;

    private:
        std::string inputBuffer;
    };

}

