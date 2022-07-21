//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Asset.h"
#include "entity/World.h"

namespace tri {

    class Map : public Asset {
    public:
        std::shared_ptr<World> world;
        std::string file;

        void setToActiveWorld();
        static void loadAndSetToActiveWorld(const std::string& file);
        virtual bool load(const std::string& file);
        virtual bool save(const std::string& file);
    };

}

