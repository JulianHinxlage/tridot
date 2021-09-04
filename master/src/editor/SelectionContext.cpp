//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "SelectionContext.h"
#include "Editor.h"
#include "tridot/components/Tag.h"
#include "tridot/engine/Serializer.h"

namespace tridot {

    void SelectionContext::select(EntityId id, bool reset) {
        if(reset) {
            entities.clear();
        }
        entities[id] = true;
        lastSelected = id;
    }

    void SelectionContext::unselect() {
        entities.clear();
    }

    void SelectionContext::unselect(EntityId id) {
        entities.erase(id);
    }

    bool SelectionContext::isSelected(EntityId id) {
        return entities.find(id) != entities.end();
    }

    EntityId SelectionContext::getSingleSelection() {
        if(entities.size() == 1){
            return entities.begin()->first;
        }else{
            return -1;
        }
    }

    void SelectionContext::destroyAll() {
        env->editor->undo.beginAction();
        for(auto &id : entities){
            env->editor->undo.destroyEntity(id.first);
            env->scene->destroy(id.first);
        }
        entities.clear();
        env->editor->undo.endAction();
    }

    EntityId SelectionContext::duplicate(EntityId id, bool addAction) {
        EntityId newId = env->scene->create();
        for(auto &type : env->reflection->getDescriptors()){
            auto *pool = env->scene->getPool(type->id());
            if(pool && pool->has(id)){
                pool->add(newId, pool->getById(id));
            }
        }
        if(env->scene->has<uuid>(newId)){
            env->scene->get<uuid>(newId).make();
        }

        if (addAction) {
            env->editor->undo.duplicateEntity(id, newId);
        }
        return newId;
    }

    void SelectionContext::duplicateAll() {
        auto tmp = entities;
        std::map<EntityId, bool> newTmp;
        unselect();

        env->editor->undo.beginAction();
        for(auto &sel : tmp){
            EntityId id = duplicate(sel.first);
            newTmp[id] = true;
            select(id, false);
        }
        env->editor->undo.endAction();
        /*
        env->editor->undo.addAction([newTmp]() {
            for (auto& sel : newTmp) {
                env->scene->destroy(sel.first);
            }
        }, [tmp]() {
            for (auto& sel : tmp) {
                env->editor->selection.select(env->editor->selection.duplicate(sel.first, false), false);
            }
        });
         */
    }

}
