//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EntityInfo.h"
#include "core/core.h"

namespace tri {

    EntityInfo::EntityInfo(const std::string &name)
        : name(name) {}

    TRI_COMPONENT(EntityInfo);
    TRI_PROPERTIES1(EntityInfo, name);

}
