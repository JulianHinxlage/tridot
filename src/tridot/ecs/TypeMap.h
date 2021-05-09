//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TYPEMAP_H
#define TRIDOT_TYPEMAP_H

#include "Reflection.h"

namespace tridot{

    class TypeMap{
    public:
        template<typename T>
        int id(){
            return id(Reflection::id<T>());
        }

        int id(int reflectId){
            while(reflectId >= map.size()){
                map.push_back(-1);
            }
            if(map[reflectId] == -1){
                map[reflectId] = count++;
            }
            return map[reflectId];
        }

        int size(){
            return count;
        }

        void clear(){
            map.clear();
            count = 0;
        }

    private:
        std::vector<int> map;
        int count = 0;
    };

}

#endif //TRIDOT_TYPEMAP_H
