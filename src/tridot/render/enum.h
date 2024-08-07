//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cstdint>

namespace tridot {

    enum Type{
        INT8,
        INT16,
        INT32,
        UINT8,
        UINT16,
        UINT32,
        FLOAT,
    };

    enum Primitive{
        POINTS,
        LINES,
        TRIANGLES,
        QUADS,
    };

    enum TextureAttachment{
        DEPTH,
        STENCIL,
        COLOR,
    };

    enum TextureFormat{
        RGBA8,
        RGB8,
        RG8,
        RED8,
        ALPHA8,
        ALPHA16,
        DEPTH16,
        DEPTH24,
        DEPTH32,
        DEPTH24STENCIL8,
    };

    enum TextureType{
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBE_MAP,
    };

    enum BufferType{
        VERTEX_BUFFER,
        INDEX_BUFFER,
        UNIFORM_BUFFER,
    };

    uint32_t internalEnum(Type type);
    uint32_t internalEnum(Primitive primitive);
    uint32_t internalEnum(TextureAttachment attachment);
    uint32_t internalEnum(TextureFormat format);
    uint32_t internalEnum(TextureType type);
    uint32_t internalEnum(BufferType type);

    uint32_t internalEnumSize(Type type);
    uint32_t internalEnumSize(Primitive primitive);
    uint32_t internalEnumSize(TextureFormat format);

}

