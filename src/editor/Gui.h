//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/Environment.h"
#include "core/Reflection.h"

namespace tri {

    class Gui {
    public:
        template<typename T>
        void setTypeGui(const std::function<void(const char *, T&)> &func){
            setTypeGui(env->reflection->getTypeId<T>(), [func](const char *label, void *ptr){
                func(label, *(T*)ptr);
            });
        }

        void drawTypeGui(int typeId, void *data);
    private:
        void setTypeGui(int typeId, const std::function<void(const char *, void*)> &func);
        void drawMember(int typeId, void *data, const char *label, bool drawHeader);
        void drawConstants(int typeId, void *data, const char *label);

        std::vector<std::function<void(const char *, void*)>> typeGuiFuncs;
    };

}

