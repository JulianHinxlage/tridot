//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "enum.h"

namespace tri {

    class Buffer {
    public:
        Buffer();
        ~Buffer();

        void bind() const;
        void unbind() const;
        static void unbind(BufferType type);

        uint32_t getId() const;
        uint32_t getElementCount() const;
        uint32_t getElementSize() const;

        void init(void *data, uint32_t size, uint32_t elementSize = 1, BufferType type = VERTEX_BUFFER, bool dynamic = false);
        void setData(void *data, uint32_t size, uint32_t offset = 0);

    private:
        uint32_t id;
        uint32_t size;
        uint32_t capacity;
        uint32_t elementSize;
        bool dynamic;
        BufferType type;
    };

}

