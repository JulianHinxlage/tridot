//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/Environment.h"
#include "core/Reflection.h"

namespace tri {

    class ComponentCache {
    public:
        std::unordered_map<std::string, std::string> data;

        template<typename T>
        void cache(T& t) {
            cache(env->reflection->getTypeId<T>(), (void*)&t);
        }

        template<typename T>
        bool uncache(T& t, bool clear = true) {
            return uncache(env->reflection->getTypeId<T>(), (void*)&t, clear);
        }

        template<typename T>
        bool isCached() {
            return isCached(env->reflection->getTypeId<T>());
        }  

        void cache(int typeId, void* ptr);
        void cache(int typeId, const std::string& str);
        void cache(const std::string &name, const std::string& str);
        bool uncache(int typeId, void* ptr, bool clear = true);
        bool uncache(const std::string& name);
        bool isCached(int typeId);
        bool isCached(const std::string& name);
    };

}
