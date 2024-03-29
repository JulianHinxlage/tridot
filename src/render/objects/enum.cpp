//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "enum.h"
#include <GL/glew.h>

namespace tri {

    uint32_t internalEnum(Type type) {
        switch (type) {
            case INT8:
                return GL_BYTE;
            case INT16:
                return GL_SHORT;
            case INT32:
                return GL_INT;
            case UINT8:
                return GL_UNSIGNED_BYTE;
            case UINT16:
                return GL_UNSIGNED_SHORT;
            case UINT32:
                return GL_UNSIGNED_INT;
            case FLOAT:
                return GL_FLOAT;
            default:
                return GL_NONE;
        }
    }

    uint32_t internalEnum(Primitive primitive) {
        switch (primitive) {
            case POINTS:
                return GL_POINTS;
            case LINES:
                return GL_LINES;
            case TRIANGLES:
                return GL_TRIANGLES;
            case QUADS:
                return GL_QUADS;
            default:
                return GL_NONE;
        }
    }

    uint32_t internalEnum(TextureAttachment attachment) {
        switch(attachment){
            case DEPTH:
                return GL_DEPTH_ATTACHMENT;
            case STENCIL:
                return GL_STENCIL_ATTACHMENT;
            case COLOR:
                return GL_COLOR_ATTACHMENT0;
            default:
                if(attachment > COLOR && attachment < COLOR + 16){
                    return GL_COLOR_ATTACHMENT0 + (attachment - COLOR);
                }else{
                    return GL_NONE;
                }
        }
    }

    uint32_t internalEnum(TextureFormat format) {
        switch (format) {
            case TEXTURE_FORMAT_NONE:
                return GL_NONE;
            case RGBA8:
                return GL_RGBA8;
            case RGB8:
                return GL_RGB8;
            case RG8:
                return GL_RG8;
            case RED8:
                return GL_R8;


            case RGB16F:
                return GL_RGB16F;
            case RGB32F:
                return GL_RGB32F;
            case RGBA16F:
                return GL_RGBA16F;
            case RGBA32F:
                return GL_RGBA32F;

            case RED16:
                return GL_R16F;
            case RED32:
                return GL_R32F;
            case RED32INT:
                return GL_R32I;
            case ALPHA8:
                return GL_ALPHA8;
            case ALPHA16:
                return GL_ALPHA16;
            case DEPTH16:
                return GL_DEPTH_COMPONENT16;
            case DEPTH24:
                return GL_DEPTH_COMPONENT24;
            case DEPTH32:
                return GL_DEPTH_COMPONENT32;
            case DEPTH24STENCIL8:
                return GL_DEPTH24_STENCIL8;
            default:
                return GL_NONE;
        }
    }

    uint32_t internalEnum(TextureType type) {
        switch (type) {
            case TEXTURE_2D:
                return GL_TEXTURE_2D;
            case TEXTURE_3D:
                return GL_TEXTURE_3D;
            case TEXTURE_CUBE_MAP:
                return GL_TEXTURE_CUBE_MAP;
            default:
                return GL_NONE;
        }
    }

    uint32_t internalEnum(BufferType type) {
        switch (type) {
            case VERTEX_BUFFER:
                return GL_ARRAY_BUFFER;
            case INDEX_BUFFER:
                return GL_ELEMENT_ARRAY_BUFFER;
            case UNIFORM_BUFFER:
                return GL_UNIFORM_BUFFER;
            default:
                return GL_NONE;
        }
    }

    uint32_t internalEnumSize(Type type) {
        switch (type) {
            case INT8:
                return 1;
            case INT16:
                return 2;
            case INT32:
                return 4;
            case UINT8:
                return 1;
            case UINT16:
                return 2;
            case UINT32:
                return 4;
            case FLOAT:
                return 4;
            default:
                return 0;
        }
    }

    uint32_t internalEnumSize(Primitive primitive) {
        switch (primitive) {
            case POINTS:
                return 1;
            case LINES:
                return 2;
            case TRIANGLES:
                return 3;
            case QUADS:
                return 4;
            default:
                return 0;
        }
    }

    uint32_t internalEnumSize(TextureFormat format) {
        switch (format) {
            case RGBA8:
                return 32;
            case RGB8:
                return 24;
            case RG8:
                return 16;
            case RED8:
                return 8;
            case RED16:
                return 16;
            case RED32:
                return 32;
            case RED32INT:
                return 32;
            case ALPHA8:
                return 8;
            case ALPHA16:
                return 16;
            case DEPTH16:
                return 16;
            case DEPTH24:
                return 24;
            case DEPTH32:
                return 32;
            default:
                return 0;
        }
    }

}