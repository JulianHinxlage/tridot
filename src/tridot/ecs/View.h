//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_VIEW_H
#define TRIDOT_VIEW_H

#include "Registry.h"

namespace ecs {

    template<typename... Components>
    class View {
    public:
        View(Registry *reg) : reg(reg){
            signature = reg->createSignature<Components...>();
            excludedSignature = 0;
            subViewCount = 1;
            subViewIndex = 0;
        }

        template<typename... ExcludedComponents>
        View &excluded(){
            excludedSignature = reg->createSignature<ExcludedComponents...>();
            return *this;
        }

        View &subView(uint32_t index, uint32_t count){
            subViewIndex = index;
            subViewCount = count;
            return *this;
        }

        template<typename Func>
        void each(const Func &func){
            Pool *pool = &reg->getEntityPool();

            if constexpr (sizeof...(Components) == 1) {
                pool = &reg->getPool<Components...>();
            }else {
                ((reg->getPool<Components>().getEntities().size() < pool->getEntities().size() ?
                        pool = &reg->getPool<Components>() : pool), ...);
            }

            int size = pool->getEntities().size();
            int startIndex = (size / subViewCount) * subViewIndex;
            int endIndex = (size / subViewCount) * (subViewIndex + 1);
            if(subViewIndex == subViewCount - 1){
                endIndex = size;
            }

            for(int i = startIndex; i < endIndex; i++){
                EntityId id = pool->getId(i);
                if constexpr (sizeof...(Components) <= 1){
                    if(excludedSignature == 0 || !reg->hasAny(id, excludedSignature)) {
                        if constexpr(std::is_invocable_v<Func, EntityId, Components &...>) {
                            func(id, *(Components*)pool->get(i)...);
                        } else {
                            func(*(Components*)pool->get(i)...);
                        }
                    }
                }else{
                    if(reg->hasAll(id, signature)){
                        if(excludedSignature == 0 || !reg->hasAny(id, excludedSignature)) {
                            if constexpr(std::is_invocable_v<Func, EntityId, Components &...>) {
                                func(id, reg->get<Components>(id)...);
                            } else {
                                func(reg->get<Components>(id)...);
                            }
                        }
                    }
                }
            }
        }

    private:
        Registry *reg;
        SignatureBitMap signature;
        SignatureBitMap excludedSignature;
        uint32_t subViewIndex;
        uint32_t subViewCount;
    };

}

#endif //TRIDOT_VIEW_H