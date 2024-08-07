//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Buffer.h"
#include "tridot/util/Ref.h"
#include <vector>

namespace tridot {

    class BatchBuffer {
    public:
        Ref<Buffer> buffer;
        std::vector<uint8_t> data;
        uint32_t elementSize;
        uint32_t elementIndex;
        uint32_t updateIndex;

        BatchBuffer();
        void init(uint32_t elementSize, BufferType type = VERTEX_BUFFER);
        void *next();
        void update();
        void reset();
        void reserve(uint32_t elementCount);
        uint32_t size();
    };

}

