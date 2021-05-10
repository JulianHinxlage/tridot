//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_POOL_H
#define TRIDOT_POOL_H

#include "tridot/core/config.h"
#include "Signal.h"
#include <memory>
#include <utility>

namespace tridot {

    class Registry;

    class Pool {
    public:
        uint32_t getIndex(EntityId id){
            uint32_t pageIndex = id >> poolPageSizeBits;
            TRI_ASSERT(pageIndex >= 0 && pageIndex < sparse.size(), "index out of bounds")
            uint32_t *page = sparse[pageIndex].get();
            if(page == nullptr){
                return -1;
            }
            uint32_t pageOffset = id & pageMask;
            return page[pageOffset];
        }

        EntityId getId(uint32_t index){
            TRI_ASSERT(index >= 0 && index < dense.size(), "index out of bounds")
            return dense[index];
        }

        bool has(EntityId id){
            uint32_t pageIndex = id >> poolPageSizeBits;
            if(pageIndex >= sparse.size()){
                return false;
            }
            uint32_t *page = sparse[pageIndex].get();
            if(page == nullptr){
                return false;
            }
            uint32_t pageOffset = id & pageMask;
            return page[pageOffset] != -1;
        }

        void *getById(EntityId id){
            return get(getIndex(id));
        }

        virtual void* get(uint32_t index){
            TRI_ASSERT(index < dense.size(), "index out of bounds")
            return &dense[index];
        }

        virtual uint32_t add(EntityId id, void *instance){
            uint32_t index = dense.size();
            uint32_t pageIndex = id >> poolPageSizeBits;
            uint32_t pageOffset = id & pageMask;
            uint32_t *page = assurePage(pageIndex);
            if(page[pageOffset] == -1){
                dense.push_back(id);
                page[pageOffset] = index;
                onAddSignal.invoke(registry, id);
                return index;
            }else{
                return page[pageOffset];
            }
        }

        virtual uint32_t remove(EntityId id){
            uint32_t index1 = getIndex(id);
            uint32_t index2 = dense.size() - 1;
            if(index1 != -1){
                onRemoveSignal.invoke(registry, id);
                EntityId id1 = getId(index1);
                EntityId id2 = getId(index2);
                dense[index1] = id2;
                dense.pop_back();
                assurePage(id2 >> poolPageSizeBits)[id2 & pageMask] = index1;
                assurePage(id1 >> poolPageSizeBits)[id1 & pageMask] = -1;
            }
            return index1;
        }

        virtual void swap(uint32_t index1, uint32_t index2){
            TRI_ASSERT(index1 >= 0 && index1 < dense.size(), "index out of bounds")
            TRI_ASSERT(index2 >= 0 && index2 < dense.size(), "index out of bounds")
            EntityId id1 = dense[index1];
            EntityId id2 = dense[index2];
            std::swap(dense[index1], dense[index2]);
            assurePage(id1 >> poolPageSizeBits)[id1 & pageMask] = index2;
            assurePage(id2 >> poolPageSizeBits)[id2 & pageMask] = index1;
        }

        const std::vector<EntityId> &getEntities(){
            return dense;
        }

        virtual void clear(){
            dense.clear();
            sparse.clear();
        }

        auto onAdd(){
            return onAddSignal.ref();
        }

        auto onRemove(){
            return onRemoveSignal.ref();
        }

        virtual void copy(const Pool &source){
            dense = source.dense;
            onAddSignal = source.onAddSignal;
            onRemoveSignal = source.onRemoveSignal;
            sparse.resize(source.sparse.size());
            registry = source.registry;
            for(int i = 0; i < source.sparse.size(); i++){
                if(source.sparse[i]){
                    sparse[i].reset(new EntityId[pageSize]);
                    for(int j = 0; j < pageSize; j++){
                        sparse[i][j] = source.sparse[i][j];
                    }
                }else{
                    sparse[i] = nullptr;
                }
            }
        }

        virtual std::shared_ptr<Pool> make(){
            return std::make_shared<Pool>();
        }

        virtual void swap(Pool &other){
            dense.swap(other.dense);
            sparse.swap(other.sparse);
            onAddSignal.swap(other.onAddSignal);
            onRemoveSignal.swap(other.onRemoveSignal);
            std::swap(registry, other.registry);
        }

    protected:
        friend class Registry;
        static const uint32_t pageMask = (1 << poolPageSizeBits) - 1;
        static const uint32_t pageSize = 1 << poolPageSizeBits;
        std::vector<EntityId> dense;
        std::vector<std::shared_ptr<uint32_t[]>> sparse;
        Signal<Registry *, EntityId> onAddSignal;
        Signal<Registry *, EntityId> onRemoveSignal;
        Registry *registry;

        uint32_t* assurePage(uint32_t pageIndex){
            if(pageIndex >= sparse.size()){
                sparse.resize(pageIndex+1);
            }
            uint32_t *page = sparse[pageIndex].get();
            if(page == nullptr){
                sparse[pageIndex].reset(new EntityId[pageSize]);
                page = sparse[pageIndex].get();
                for(int i = 0; i < pageSize; i++){
                    page[i] = -1;
                }
            }
            return page;
        }
    };

}

#endif //TRIDOT_POOL_H
