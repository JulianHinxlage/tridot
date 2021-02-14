//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_VERTEXARRAY_H
#define TRIDOT_VERTEXARRAY_H

#include "tridot/core/Ref.h"
#include "Buffer.h"
#include <vector>

namespace tridot {

    enum Type{
        NONE,
        INT8,
        INT16,
        INT32,
        UINT8,
        UINT16,
        UINT32,
        FLOAT,
    };

    enum Mode{
        POINTS,
        LINES,
        TRIANGLES,
        QUADS,
    };

    class Attribute {
    public:
        Type type;
        int count;
        int size;
        int offset;

        Attribute(Type type = NONE, int count = 1);
    };

    class VertexArray {
    public:
        VertexArray();
        explicit VertexArray(const VertexArray &vertexArray);
        ~VertexArray();

        void bind();
        static void unbind();
        uint32_t getId();

        void addIndexBuffer(const Ref<Buffer> &indexBuffer, Type type);
        void addVertexBuffer(const Ref<Buffer> &vertexBuffer, std::vector<Attribute> layout, int divisor = 0);
        void submit(int count = -1);
        void setMode(Mode mode);

    private:
        uint32_t id;
        uint32_t nextAttribute;
        Mode mode;

        class IBuffer{
        public:
            Ref<Buffer> indexBuffer;
            Type type;
        };
        std::vector<IBuffer> indexBuffer;

        class VBuffer{
        public:
            Ref<Buffer> vertexBuffer;
            std::vector<Attribute> layout;
            int divisor;
        };
        std::vector<VBuffer> vertexBuffer;
    };

}

#endif //TRIDOT_VERTEXARRAY_H
