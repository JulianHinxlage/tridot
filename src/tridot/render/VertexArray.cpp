//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "VertexArray.h"
#include "tridot/core/Log.h"
#include <GL/glew.h>

namespace tridot {

    GLenum glType(Type type){
        switch (type) {
            case NONE:
                return GL_NONE;
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

    GLenum glMode(Mode mode){
        switch (mode) {
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

    Attribute::Attribute(Type type, int count) {
        this->type = type;
        this->count = count;
        offset = 0;
        switch (type) {
            case NONE:
                size = 0;
                break;
            case INT8:
                size = 1;
                break;
            case INT16:
                size = 2;
                break;
            case INT32:
                size = 4;
                break;
            case UINT8:
                size = 1;
                break;
            case UINT16:
                size = 2;
                break;
            case UINT32:
                size = 4;
                break;
            case FLOAT:
                size = 4;
                break;
            default:
                size = 0;
                break;
        }
    }

    VertexArray::VertexArray() {
        id = 0;
        nextAttribute = 0;
        mode = TRIANGLES;
    }

    VertexArray::VertexArray(const VertexArray &vertexArray) : VertexArray() {
        for(auto &buffer : vertexArray.indexBuffer){
            addIndexBuffer(buffer.indexBuffer, buffer.type);
        }
        for(auto &buffer : vertexArray.vertexBuffer){
            addVertexBuffer(buffer.vertexBuffer, buffer.layout, buffer.divisor);
        }
    }

    VertexArray::~VertexArray() {
        if(id != 0){
            glDeleteVertexArrays(1, &id);
            Log::debug("deleted vertex array ", id);
            id = 0;
        }
    }

    void bindVertexArray(uint32_t id){
        static uint32_t currentId = 0;
        if(currentId != id){
            glBindVertexArray(id);
            currentId = id;
        }
    }

    void VertexArray::bind() {
        bindVertexArray(id);
    }

    void VertexArray::unbind() {
        bindVertexArray(0);
    }

    uint32_t VertexArray::getId() {
        return id;
    }

    void VertexArray::addIndexBuffer(const Ref<Buffer> &indexBuffer, Type type) {
        if(id == 0){
            glGenVertexArrays(1, &id);
            Log::debug("created vertex array ", id);
        }
        bind();
        indexBuffer->unbind();
        indexBuffer->bind();
        this->indexBuffer.push_back({indexBuffer, type});
        unbind();
    }

    void VertexArray::addVertexBuffer(const Ref<Buffer> &vertexBuffer, std::vector<Attribute> layout, int divisor) {
        if(id == 0){
            glGenVertexArrays(1, &id);
            Log::debug("created vertex array ", id);
        }
        bind();
        vertexBuffer->unbind();
        vertexBuffer->bind();
        this->vertexBuffer.push_back({vertexBuffer, layout, divisor});

        int stride = 0;
        for(auto &a : layout){
            a.offset = stride;
            stride += a.size * a.count;
        }

        for(auto &a : layout){
            glEnableVertexAttribArray(nextAttribute);
            glVertexAttribPointer(nextAttribute, a.count, glType(a.type), GL_FALSE, stride, (void*)(size_t)a.offset);
            glVertexAttribDivisor(nextAttribute, divisor);
            nextAttribute++;
        }
        unbind();
    }

    void VertexArray::submit(int count) {
        bind();
        if(count == -1){
            if(indexBuffer.empty()){
                if(!vertexBuffer.empty()){
                    count = vertexBuffer[0].vertexBuffer->getElementCount();
                }
            }else{
                count = indexBuffer[0].indexBuffer->getElementCount();
            }
        }

        if(indexBuffer.empty()){
            glDrawArrays(glMode(mode), 0, count);
        }else{
            glDrawElements(glMode(mode), count, glType(indexBuffer[0].type), nullptr);
        }
    }

    void VertexArray::setMode(Mode mode) {
        this->mode = mode;
    }

}