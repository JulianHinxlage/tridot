//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TYPEMAP_H
#define TRIDOT_TYPEMAP_H

#include "tridot/core/Environment.h"
#include <utility>

namespace tridot{

    class TypeMap{
    public:
        template<typename T>
        int id(){
            return id(env->reflection->getTypeId<T>());
        }

        int id(int typeId){
            while(typeId >= map.size()){
                map.push_back(-1);
            }
            if(map[typeId] == -1){
                map[typeId] = count++;
            }
            return map[typeId];
        }

        int size(){
            return count;
        }

        void clear(){
            map.clear();
            count = 0;
        }

        void swap(TypeMap &other){
            map.swap(other.map);
            std::swap(count, other.count);
        }

    private:
        std::vector<int> map;
        int count = 0;
    };

}

#endif //TRIDOT_TYPEMAP_H
