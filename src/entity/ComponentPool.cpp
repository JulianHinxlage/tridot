//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentPool.h"

namespace tri {

    ComponentPool::ComponentPool(int typeId, int elementSize){
        elements.elementSize = elementSize;
        elements.typeId = typeId;
    }

    ComponentPool::ComponentPool(const ComponentPool& pool){
        copy(pool);
    }

    ComponentPool::~ComponentPool(){
        clear();
    }

    void* ComponentPool::getElementById(EntityId id){
        return getElementByIndex(getIndexById(id));
    }

    void* ComponentPool::getElementByIndex(EntityId index) {
        TRI_ASSERT(index < elements.size, "index out of bounds");
        return (void*)elements[index];
    }

    EntityId ComponentPool::getIndexById(EntityId id) const {
        EntityId pageIndex = id >> pageSizeBits;
        EntityId elementIndex = id & ((1u << pageSizeBits) - 1);
        TRI_ASSERT(pageIndex < sparse.size() && sparse[pageIndex].data, "index out of bounds");
        return sparse[pageIndex].data[elementIndex];
    }

    EntityId ComponentPool::getIdByIndex(EntityId index) const {
        TRI_ASSERT(index < dense.size(), "index out of bounds");
        return dense[index];
    }

    void* ComponentPool::add(EntityId id){
        EntityId pageIndex = id >> pageSizeBits;
        EntityId elementIndex = id & ((1u << pageSizeBits) - 1);

        if (pageIndex >= sparse.size()) {
            sparse.resize(pageIndex + 1);
        }
        Page &page = sparse[pageIndex];
        if (page.data == nullptr) {
            page.data.reset(new EntityId[1u << pageSizeBits]);
            page.entryCount = 0;
            for (EntityId i = 0; i < (1u << pageSizeBits); i++) {
                page.data[i] = -1;
            }
        }
        if (page.data[elementIndex] != -1) {
            return getElementByIndex(page.data[elementIndex]);
        }

        EntityId index = (EntityId)dense.size();
        page.data[elementIndex] = index;
        page.entryCount++;
        dense.push_back(id);
        elements.resize(elements.size + 1);
        return elements[index];
    }

    bool ComponentPool::has(EntityId id) const {
        EntityId pageIndex = id >> pageSizeBits;
        EntityId elementIndex = id & ((1u << pageSizeBits) - 1);
        if (pageIndex < sparse.size() && sparse[pageIndex].data) {
            return sparse[pageIndex].data[elementIndex] != -1;
        }
        else {
            return false;
        }
    }

    bool ComponentPool::remove(EntityId id) {
        EntityId pageIndex = id >> pageSizeBits;
        EntityId elementIndex = id & ((1u << pageSizeBits) - 1);

        if (pageIndex >= sparse.size()) {
            return false;
        }
        Page &page = sparse[pageIndex];
        if (page.data == nullptr) {
            return false;
        }
        EntityId index = page.data[elementIndex];
        if (index == -1) {
            return false;
        }

        EntityId swapId = dense.back();
        EntityId swapPageIndex = swapId >> pageSizeBits;
        EntityId swapElementIndex = swapId & ((1u << pageSizeBits) - 1);
        sparse[swapPageIndex].data[swapElementIndex] = index;
        page.data[elementIndex] = -1;
        page.entryCount--;

        if (page.entryCount <= 0) {
            page.data = nullptr;
        }

        std::swap(dense[index], dense.back());
        elements.swap(index, dense.size() - 1);
        dense.pop_back();
        elements.resize(elements.size - 1);
        return true;
    }

    int ComponentPool::size() const {
        return (int)dense.size();
    }

    void* ComponentPool::elementData() {
        return elements.data;
    }

    EntityId* ComponentPool::idData() {
        return dense.data();
    }

    void ComponentPool::clear() {
        elements.clear();
        dense.clear();
        sparse.clear();
    }

    void ComponentPool::swap(EntityId index1, EntityId index2) {
        TRI_ASSERT(index1 < dense.size(), "index out of bounds");
        TRI_ASSERT(index2 < dense.size(), "index out of bounds");

        EntityId id1 = getIdByIndex(index1);
        EntityId id2 = getIdByIndex(index2);
        EntityId pageIndex1 = id1 >> pageSizeBits;
        EntityId elementIndex1 = id1 & ((1u << pageSizeBits) - 1);
        EntityId pageIndex2 = id2 >> pageSizeBits;
        EntityId elementIndex2 = id2 & ((1u << pageSizeBits) - 1);
        std::swap(sparse[pageIndex1].data[elementIndex1], sparse[pageIndex2].data[elementIndex2]);
        std::swap(dense[index1], dense[index2]);
        elements.swap(index1, index2);
    }

    void ComponentPool::copy(const ComponentPool &from) {
        elements.copy(from.elements);
        dense = from.dense;
        sparse.resize(from.sparse.size());
        for (int i = 0; i < sparse.size(); i++) {
            if (from.sparse[i].data) {
                sparse[i].data.reset(new EntityId[1u << pageSizeBits]);
                sparse[i].entryCount = from.sparse[i].entryCount;
                for (int j = 0; j < (1u << pageSizeBits); j++) {
                    sparse[i].data[j] = from.sparse[i].data[j];
                }
            }
            else {
                sparse[i].data = nullptr;
                sparse[i].entryCount = 0;
            }
        }
    }

    void ComponentPool::swap(ComponentPool &pool) {
        elements.swap(pool.elements);
        dense.swap(pool.dense);
        sparse.swap(pool.sparse);
    }

    ///////////////
    /// Storage ///
    ///////////////

    ComponentPool::Storage::Storage(){
        data = nullptr;
        size = 0;
        capacity = 0;
        elementSize = 0;
        typeId = 0;
    }

    void ComponentPool::Storage::clear(){
        auto *desc = env->reflection->getType(typeId);
        desc->destruct(data, capacity);
        delete[] data;
        data = nullptr;
        size = 0;
        capacity = 0;
    }

    void *ComponentPool::Storage::operator[](int index) const {
        return data + index * elementSize;
    }

    void ComponentPool::Storage::swap(Storage &storage){
        std::swap(elementSize, storage.elementSize);
        std::swap(size, storage.size);
        std::swap(data, storage.data);
        std::swap(capacity, storage.capacity);
        std::swap(typeId, storage.typeId);
    }

    void ComponentPool::Storage::swap(int index1, int index2){
        auto *desc = env->reflection->getType(typeId);
        desc->swap(data + index1 * elementSize, data + index2 * elementSize);
    }

    void ComponentPool::Storage::copy(const Storage &from){
        if(capacity > 0){
            auto *desc = env->reflection->getType(typeId);
            desc->destruct(data, capacity);
            delete[] data;
        }
        elementSize = from.elementSize;
        typeId = from.typeId;
        size = from.size;
        capacity = from.capacity;
        data = new uint8_t[capacity * elementSize];
        auto *desc = env->reflection->getType(typeId);
        desc->construct(data, capacity);
        desc->copy(from.data, data, capacity);
    }

    void ComponentPool::Storage::resize(int newSize){
        if(newSize > capacity){
            int newCapacity = capacity << 1;
            if(newCapacity == 0){
                newCapacity = 1;
            }
            uint8_t *newData = new uint8_t[newCapacity * elementSize];
            auto *desc = env->reflection->getType(typeId);
            desc->move(data, newData, capacity);
            desc->destruct(data, capacity);
            desc->construct(newData + capacity * elementSize, newCapacity - capacity);
            delete[] data;
            data = newData;
            size = newSize;
            capacity = newCapacity;
        }else if(newSize < (capacity >> 1)) {
            int newCapacity = capacity >> 1;
            uint8_t *newData = new uint8_t[newCapacity * elementSize];
            auto *desc = env->reflection->getType(typeId);
            desc->move(data, newData, newCapacity);
            desc->destruct(data, capacity);
            delete[] data;
            data = newData;
            size = newSize;
            capacity = newCapacity;
        }else if(newSize == 0){
            auto *desc = env->reflection->getType(typeId);
            desc->destruct(data, capacity);
            delete[] data;
            data = nullptr;
            size = newSize;
            capacity = 0;
        }else{
            size = newSize;
        }
    }

}