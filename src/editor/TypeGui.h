//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "core/Environment.h"
#include "core/Reflection.h"

namespace tri {

    class TypeGui {
    public:
        template<typename T>
        void setTypeFunction(const std::function<void(const char *, T&, T *min, T *max)> &func){
            setTypeFunction(env->reflection->getTypeId<T>(), [func](const char *label, void *ptr, void *min, void *max){
                func(label, *(T*)ptr, (T*)min, (T*)max);
            });
        }

        void drawType(int typeId, void *data);

    private:
        void setTypeFunction(int typeId, const std::function<void(const char *, void*, void*, void*)> &func);
        void drawMember(int typeId, void *data, const char *label, void *min, void *max , bool drawHeader);
        void drawConstants(int typeId, void *data, const char *label);

        std::vector<std::function<void(const char *, void*, void*, void*)>> typeFunctions;
    };

}

