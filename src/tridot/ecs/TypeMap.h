//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TYPEMAP_H
#define TRIDOT_TYPEMAP_H

#include "Reflection.h"

namespace ecs{

    class TypeMap{
    public:
        template<typename T>
        int id(){
            int reflectId = Reflection::id<T>();
            while(reflectId >= map.size()){
                map.push_back(-1);
            }
            if(map[reflectId] == -1){
                map[reflectId] = count++;
            }
            return map[reflectId];
        }

        int id(int reflectId){
            if(reflectId >= map.size()){
                return -1;
            }
            return map[reflectId];
        }

        int size(){
            return count;
        }

    private:
        std::vector<int> map;
        int count = 0;
    };

}

#endif //TRIDOT_TYPEMAP_H
