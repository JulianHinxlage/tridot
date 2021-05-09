//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Scene.h"
#include "Engine.h"
#include "Serializer.h"

namespace tridot {

    bool Scene::load() {
        Serializer serializer;
        bool valid = serializer.load(file, *this, engine.resources);
        if(valid){
            Log::debug("loaded scene ", file);
        }
        return valid;
    }

    bool Scene::save() {
        Serializer serializer;
        return serializer.save(file, *this, engine.resources);
    }

    bool Scene::load(const std::string &file) {
        this->file = file;
        return load();
    }

    bool Scene::save(const std::string &file) {
        this->file = file;
        return save();
    }

    bool Scene::preLoad(const std::string &file) {
        active = false;
        bool valid = load(file);
        if(!valid){
            active = true;
        }
        return valid;
    }

    bool Scene::postLoad() {
        active = true;
        return true;
    }

    void Scene::copy(const Scene &source) {
        active = source.active;
        Registry::copy(source);
    }

}
