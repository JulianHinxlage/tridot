//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "SelectionContext.h"
#include "Editor.h"
#include "tridot/engine/Engine.h"
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
        Editor::undo.beginAction();
        for(auto &id : entities){
            Editor::undo.destroyEntity(id.first);
            engine.destroy(id.first);
        }
        entities.clear();
        Editor::undo.endAction();
    }

    EntityId SelectionContext::duplicate(EntityId id, bool addAction) {
        EntityId newId = engine.create();
        for(auto &type : env->reflection->getDescriptors()){
            auto *pool = engine.getPool(type->id());
            if(pool && pool->has(id)){
                pool->add(newId, pool->getById(id));
            }
        }
        if(engine.has<uuid>(newId)){
            engine.get<uuid>(newId).make();
        }

        if (addAction) {
            Editor::undo.duplicateEntity(id, newId);
        }
        return newId;
    }

    void SelectionContext::duplicateAll() {
        auto tmp = entities;
        std::map<EntityId, bool> newTmp;
        unselect();

        Editor::undo.beginAction();
        for(auto &sel : tmp){
            EntityId id = duplicate(sel.first);
            newTmp[id] = true;
            select(id, false);
        }
        Editor::undo.endAction();
        /*
        Editor::undo.addAction([newTmp]() {
            for (auto& sel : newTmp) {
                engine.destroy(sel.first);
            }
        }, [tmp]() {
            for (auto& sel : tmp) {
                Editor::selection.select(Editor::selection.duplicate(sel.first, false), false);
            }
        });
         */
    }

}
