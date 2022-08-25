//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Guid.h"

namespace tri {

    class EntityInfo {
    public:
        std::string name;
        Guid guid;

        EntityInfo(const std::string &name = "");
    };

}

