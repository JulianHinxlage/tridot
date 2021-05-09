//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SCENE_H
#define TRIDOT_SCENE_H

#include "tridot/ecs/Registry.h"

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

        template<typename... Components>
        View<Components...> view(){
            if(active){
                return View<Components...>(this);
            }else{
                return View<Components...>(nullptr);
            }
        }

        template<typename... Components, typename Func>
        void each(const Func &func){
            view<Components...>().each(func);
        }

        Pool &getEntityPool(){
            if(active){
                return entityPool;
            }else{
                return emptyPool;
            }
        }

    private:
        Pool emptyPool;
        bool active = true;
    };

}

#endif //TRIDOT_SCENE_H
