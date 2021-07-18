//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SCENE_H
#define TRIDOT_SCENE_H

#include "tridot/entity/Registry.h"

namespace tridot {

    class Scene : public Registry{
    public:
        std::string name;
        std::string file;

        bool load();
        bool save();
        bool load(const std::string &file);
        bool save(const std::string &file);
        bool preLoad(const std::string &file);
        bool postLoad();

        void copy(const Scene &source);
        void swap(Scene &other);
    };

}

#endif //TRIDOT_SCENE_H
