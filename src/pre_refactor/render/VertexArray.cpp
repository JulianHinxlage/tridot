//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "VertexArray.h"
#include "core/core.h"
#include <GL/glew.h>

namespace tri {

    Attribute::Attribute(Type type, int count, bool normalized) {
        this->type = type;
        this->count = count;
        this->normalized = normalized;
        offset = 0;
        size = internalEnumSize(type);
    }

    VertexArray::VertexArray() {
        id = 0;
        nextAttribute = 0;
        primitive = TRIANGLES;
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
        clear();
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
            glVertexAttribPointer(nextAttribute, a.count, internalEnum(a.type), a.normalized ? GL_TRUE : GL_FALSE, stride, (void*)(size_t)a.offset);
            glVertexAttribDivisor(nextAttribute, divisor);
            nextAttribute++;
        }
        unbind();
    }

    void VertexArray::submit(int vertexCount, int instanceCount) {
        bind();
        if(vertexCount == -1){
            if(indexBuffer.empty()){
                if(!vertexBuffer.empty()){
                    vertexCount = vertexBuffer[0].vertexBuffer->getElementCount();
                }
            }else{
                vertexCount = indexBuffer[0].indexBuffer->getElementCount();
            }
        }

        if(instanceCount == -1) {
            if (indexBuffer.empty()) {
                glDrawArrays(internalEnum(primitive), 0, vertexCount);
            } else {
                glDrawElements(internalEnum(primitive), vertexCount, internalEnum(indexBuffer[0].type), nullptr);
            }
        }else{
            if (indexBuffer.empty()) {
                glDrawArraysInstanced(internalEnum(primitive), 0, vertexCount, instanceCount);
            } else {
                glDrawElementsInstanced(internalEnum(primitive), vertexCount, internalEnum(indexBuffer[0].type), nullptr, instanceCount);
            }
        }
        unbind();
    }

    void VertexArray::setPrimitive(Primitive primitive) {
        this->primitive = primitive;
    }

    void VertexArray::clear() {
        if(id != 0){
            glDeleteVertexArrays(1, &id);
            id = 0;
        }
        nextAttribute = 0;
        vertexBuffer.clear();
        indexBuffer.clear();
    }

}