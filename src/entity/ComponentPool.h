//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

    class ComponentPool {
    public:
        static const uint32_t pageSizeBits = 8;

        ComponentPool(int typeId = 0, int elementSize = 1);
        ComponentPool(const ComponentPool &pool);
        ~ComponentPool();

        void* getElementById(EntityId id);
        void* getElementByIndex(EntityId index);

        EntityId getIndexById(EntityId id) const;
        EntityId getIdByIndex(EntityId index) const;

        void* add(EntityId id);
        bool has(EntityId id) const;
        bool remove(EntityId id);

        int size() const;
        void* elementData();
        EntityId* idData();
        void clear();
        void swap(EntityId index1, EntityId index2);
        void copy(const ComponentPool& from);
        void swap(ComponentPool &pool);

    private:
        class Storage{
        public:
            int typeId;
            int elementSize;
            int size;
            int capacity;
            uint8_t *data;

            Storage();
            void clear();
            void *operator[](int index) const;
            void swap(Storage &storage);
            void swap(int index1, int index2);
            void copy(const Storage &from);
            void resize(int newSize);
        };
        Storage elements;
        std::vector<EntityId> dense;

        class Page {
        public:
            std::shared_ptr<EntityId[]> data;
            int entryCount;
        };
        std::vector<Page> sparse;
    };

}