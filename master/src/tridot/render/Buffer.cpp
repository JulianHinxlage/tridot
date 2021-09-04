//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/Environment.h"
#include "Buffer.h"
#include <GL/glew.h>

namespace tridot {

    Buffer::Buffer() {
        id = 0;
        size = 0;
        capacity = 0;
        elementSize = 1;
        dynamic = false;
        type = VERTEX_BUFFER;
    }

    Buffer::~Buffer() {
        if(id != 0){
            glDeleteBuffers(1, &id);
            env->console->trace("deleted buffer ", id);
            id = 0;
        }
    }

    void bindBuffer(uint32_t id, BufferType type){
        static uint32_t currentId[32];
        if(currentId[(int)type] != id){
            glBindBuffer(internalEnum(type), id);
            currentId[(int)type] = id;
        }
    }

    void Buffer::bind() const {
        bindBuffer(id, type);
    }

    void Buffer::unbind() const {
        bindBuffer(0, type);
    }

    void Buffer::unbind(BufferType type) {
        bindBuffer(0, type);
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

    void Buffer::init(void *data, uint32_t size, uint32_t elementSize, BufferType type, bool dynamic) {
        this->elementSize = elementSize;
        this->dynamic = dynamic;
        this->type = type;
        if(id == 0){
            glGenBuffers(1, &id);
            env->console->trace("created buffer ", id);
        }
        bind();
        glBufferData(internalEnum(type), size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        this->capacity = size;
        if(data == nullptr){
            this->size = 0;
        }else{
            this->size = size;
        }
        unbind();
    }

    void Buffer::setData(void *data, uint32_t size, uint32_t offset) {
        bind();
        if(this->capacity < size + offset){
            glBufferData(internalEnum(type), size + offset, nullptr, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            glBufferSubData(internalEnum(type), offset, size, data);
            this->capacity = size + offset;
        }else{
            glBufferSubData(internalEnum(type), offset, size, data);
        }
        if(size + offset > this->size){
            this->size = size + offset;
        }
        unbind();
    }

}