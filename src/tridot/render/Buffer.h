//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_BUFFER_H
#define TRIDOT_BUFFER_H

#include <cstdint>

namespace tridot {

    class Buffer {
    public:
        Buffer();
        ~Buffer();

        void bind() const;
        void unbind() const;
        static void unbind(bool indexBuffer);

        uint32_t getId() const;
        uint32_t getElementCount() const;
        uint32_t getElementSize() const;

        void init(void *data, uint32_t size, uint32_t elementSize = 1, bool indexBuffer = false, bool dynamic = false);
        void setData(void *data, uint32_t size, int offset = 0);

    private:
        uint32_t id;
        uint32_t size;
        uint32_t capacity;
        uint32_t elementSize;
        bool dynamic;
        bool indexBuffer;
    };

}

#endif //TRIDOT_BUFFER_H
