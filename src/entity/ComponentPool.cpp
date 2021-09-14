#include "ComponentPool.h"
#include "ComponentPool.h"
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentPool.h"

namespace tri {

    ComponentPool::ComponentPool(int typeId, int elementSize){
        this->elementSize = elementSize;
        this->typeId = typeId;
    }

    ComponentPool::ComponentPool(const ComponentPool& pool){
        operator=(pool);
    }

    ComponentPool::~ComponentPool(){
        clear();
    }

    void* ComponentPool::getElementById(EntityId id){
        return getElementByIndex(getIndexById(id));
    }

    void* ComponentPool::getElementByIndex(EntityId index) {
        TRI_ASSERT(index < elements.size() / elementSize, "index out of bounds");
        return (void*)(elements.data() + index * elementSize);
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
        elements.resize(elements.size() + elementSize);
        return elements.data() + index * (size_t)elementSize;
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
        for (int i = 0; i < elementSize; i++) {
            std::swap(elements[index * (size_t)elementSize + i], elements[elements.size() - elementSize + i]);
        }

        dense.pop_back();
        elements.resize(elements.size() - elementSize);
        return true;
    }

    int ComponentPool::size() const {
        return (int)dense.size();
    }

    void* ComponentPool::elementData() {
        return elements.data();
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
        for (int i = 0; i < elementSize; i++) {
            std::swap(elements[index1 * (size_t)elementSize + i], elements[index2 * (size_t)elementSize + i]);
        }
    }

    void ComponentPool::operator=(const ComponentPool& pool){
        elementSize = pool.elementSize;
        typeId = pool.typeId;
        elements = pool.elements;
        dense = pool.dense;
        sparse.resize(pool.sparse.size());
        for (int i = 0; i < sparse.size(); i++) {
            if (pool.sparse[i].data) {
                sparse[i].data.reset(new EntityId[1u << pageSizeBits]);
                sparse[i].entryCount = pool.sparse[i].entryCount;
                for (int j = 0; j < (1u << pageSizeBits); j++) {
                    sparse[i].data[j] = pool.sparse[i].data[j];
                }
            }
            else {
                sparse[i].data = nullptr;
                sparse[i].entryCount = 0;
            }
        }
    }

}