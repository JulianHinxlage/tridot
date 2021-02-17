//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/Log.h"
#include "Buffer.h"
#include <GL/glew.h>

namespace tridot {

    Buffer::Buffer() {
        id = 0;
        size = 0;
        capacity = 0;
        elementSize = 1;
        dynamic = false;
        indexBuffer = false;
    }

    Buffer::~Buffer() {
        if(id != 0){
            glDeleteBuffers(1, &id);
            Log::trace("deleted buffer ", id);
            id = 0;
        }
    }

    void bindVertexBuffer(uint32_t id){
        static uint32_t currentId = 0;
        if(currentId != id){
            glBindBuffer(GL_ARRAY_BUFFER, id);
            currentId = id;
        }
    }

    void bindIndexBuffer(uint32_t id){
        static uint32_t currentId = 0;
        if(currentId != id){
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
            currentId = id;
        }
    }

    void Buffer::bind() const {
        if(indexBuffer){
            bindIndexBuffer(id);
        }else {
            bindVertexBuffer(id);
        }
    }

    void Buffer::unbind() const {
        if(indexBuffer){
            bindIndexBuffer(0);
        }else {
            bindVertexBuffer(0);
        }
    }

    void Buffer::unbind(bool indexBuffer) {
        if(indexBuffer){
            bindIndexBuffer(0);
        }else {
            bindVertexBuffer(0);
        }
    }

    uint32_t Buffer::getId() const {
        return id;
    }

    uint32_t Buffer::getElementSize() const {
        return elementSize;
    }

    uint32_t Buffer::getElementCount() const {
        return size / elementSize;
    }

    void Buffer::init(void *data, uint32_t size, uint32_t elementSize, bool indexBuffer, bool dynamic) {
        this->elementSize = elementSize;
        this->dynamic = dynamic;
        this->indexBuffer = indexBuffer;
        if(id == 0){
            glGenBuffers(1, &id);
            Log::trace("created buffer ", id);
            bind();
        }
        glNamedBufferData(id, size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        this->capacity = size;
        if(data == nullptr){
            this->size = 0;
        }else{
            this->size = size;
        }
    }

    void Buffer::setData(void *data, uint32_t size, int offset) {
        if(this->capacity < size + offset){
            glNamedBufferData(id, size + offset, nullptr, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            glNamedBufferSubData(id, offset, size, data);
            this->capacity = size + offset;
        }else{
            glNamedBufferSubData(id, offset, size, data);
        }
        if(size + offset > this->size){
            this->size = size + offset;
        }
    }

}