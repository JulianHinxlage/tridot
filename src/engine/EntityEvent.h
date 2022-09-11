//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/Reflection.h"
#include "core/config.h"
#include "MetaTypes.h"

namespace tri {

    class EntityEvent {
    public:
        class Listener {
        public:
            EntityId entityId = -1;
            FunctionIdentifier function;
        };
        std::vector<Listener> listeners;

        void invoke();
        void addListener(EntityId id, int classId, int functionIndex);
    };

}

