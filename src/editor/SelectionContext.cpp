//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "SelectionContext.h"
#include "Editor.h"
#include "tridot/engine/Engine.h"
#include "tridot/components/Tag.h"
#include "tridot/engine/Serializer.h"

namespace tridot {

    void SelectionContext::select(ecs::EntityId id, bool reset) {
        if(reset) {
            selectedEntities.clear();
        }
        selectedEntities[id] = true;
        lastSelected = id;
    }

    void SelectionContext::unselect() {
        selectedEntities.clear();
    }

    void SelectionContext::unselect(ecs::EntityId id) {
        selectedEntities.erase(id);
    }

    bool SelectionContext::isSelected(ecs::EntityId id) {
        return selectedEntities.find(id) != selectedEntities.end();
    }

    ecs::EntityId SelectionContext::getSingleSelection() {
        if(selectedEntities.size() == 1){
            return selectedEntities.begin()->first;
        }else{
            return -1;
        }
    }

    void SelectionContext::destroyAll() {
        for(auto &id : selectedEntities){
            engine.destroy(id.first);
        }
        selectedEntities.clear();
    }

    ecs::EntityId SelectionContext::duplicate(ecs::EntityId id, bool addAction) {
        ecs::EntityId newId = engine.create();
        for(auto &type : ecs::Reflection::getTypes()){
            auto *pool = engine.getPool(type->id());
            if(pool && pool->has(id)){
                pool->add(newId, pool->getById(id));
            }
        }
        if(engine.has<uuid>(newId)){
            engine.get<uuid>(newId).make();
        }

        if (addAction) {
            Editor::undo.addAction([newId]() {
                engine.destroy(newId);
            }, [id]() {
                Editor::selection.duplicate(id, false);
            });
        }

        return newId;
    }

    void SelectionContext::duplicateAll() {
        auto tmp = selectedEntities;
        std::map<ecs::EntityId, bool> newTmp;
        unselect();
        for(auto &sel : tmp){
            ecs::EntityId id = duplicate(sel.first, false);
            newTmp[id] = true;
            select(id, false);
        }

        Editor::undo.addAction([newTmp]() {
            for (auto& sel : newTmp) {
                engine.destroy(sel.first);
            }
        }, [tmp]() {
            for (auto& sel : tmp) {
                Editor::selection.select(Editor::selection.duplicate(sel.first, false), false);
            }
        });
    }

}
