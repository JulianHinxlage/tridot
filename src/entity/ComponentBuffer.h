//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

    //stores a single instance of an component as a dynamic type
    class ComponentBuffer {
    public:
        ComponentBuffer();
        ComponentBuffer(int typeId);
        void set(int typeId, void *ptr = nullptr);
        void *get();
        void get(void *ptr);
        int getTypeId();
        bool isSet();
        void clear();
    private:
        std::vector<uint8_t> data;
        int typeId;
    };

}

