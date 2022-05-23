//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentBuffer.h"
#include "core/core.h"

namespace tri {

    ComponentBuffer::ComponentBuffer() {
        typeId = -1;
    }

    ComponentBuffer::ComponentBuffer(int typeId) {
        set(typeId);
    }

    ComponentBuffer::ComponentBuffer(const ComponentBuffer& buffer) {
        operator=(buffer);
    }

    ComponentBuffer::ComponentBuffer(ComponentBuffer&& buffer)
        : typeId(buffer.typeId), data(std::move(buffer.data)) {}
    
    ComponentBuffer::~ComponentBuffer() {
        clear();
    }

    void ComponentBuffer::operator=(const ComponentBuffer& buffer) {
        set(buffer.typeId, buffer.data.data());
    }

    void ComponentBuffer::set(int typeId, const void *ptr) {
        clear();
        this->typeId = typeId;
        auto *desc = env->reflection->getType(typeId);
        if (desc) {
            data.resize(desc->size);
            if (ptr != nullptr) {
                desc->copy(ptr, data.data());
            }
            else {
                desc->construct(data.data());
            }
        }
    }

    void *ComponentBuffer::get() {
        return data.data();
    }

    void ComponentBuffer::get(void *ptr) {
        auto *desc = env->reflection->getType(typeId);
        if (desc) {
            desc->copy(data.data(), ptr);
        }
    }

    int ComponentBuffer::getTypeId() {
        return typeId;
    }

    bool ComponentBuffer::isSet() {
        return typeId != -1;
    }

    void ComponentBuffer::clear() {
        auto* desc = env->reflection->getType(typeId);
        if (desc) {
            if (data.size() == desc->size) {
                desc->destruct(data.data());
            }
        }
        typeId = -1;
        data.clear();
    }

}
