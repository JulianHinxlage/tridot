//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/util/Guid.h"

namespace tri {

    class Asset {
    public:
        int assetIndex = -1;
        Guid guid;

        Asset(){}
        virtual ~Asset(){}
        virtual bool load(const std::string &file){return true;}
        virtual bool loadActivate(){return true;}
        virtual bool save(const std::string &file){return false;}
    };

}

