//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    class EditorElement {
    public:
        std::string name;
        enum Type{
            WINDOW,
            DEBUG_WINDOW,
            ELEMENT,
            ALWAYS_OPEN,
        };
        Type type;
        bool isOpen = false;

        virtual void startup() {}
        virtual void update() {}
        virtual void shutdown() {}
    };

}

