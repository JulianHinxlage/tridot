//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EntityInfo.h"
#include "core/core.h"
#include "Random.h"

namespace tri {

    EntityInfo::EntityInfo(const std::string &name)
        : name(name) {
        guid = env->random->getGuid();
    }

    TRI_CLASS_FLAGS(EntityInfo, "EntityInfo", "", (ClassDescriptor::Flags)(ClassDescriptor::COMPONENT | ClassDescriptor::HIDDEN));
    TRI_PROPERTIES2(EntityInfo, name, guid);

}
