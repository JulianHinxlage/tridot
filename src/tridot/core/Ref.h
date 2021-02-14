//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_REF_H
#define TRIDOT_REF_H

#include <memory>

namespace tridot {

    template<typename T>
    class Ref : public std::shared_ptr<T>{
    public:
        Ref(){}
        Ref(bool make){if(make){*this = this->make();}}
        Ref(T *ptr) : std::shared_ptr<T>(ptr, [](T *ptr){}){}
        Ref(const std::shared_ptr<T> &ptr) : std::shared_ptr<T>(ptr){}
        Ref(const Ref<T> &ref) : std::shared_ptr<T>(ref){}
        template<typename... Args>
        static Ref<T> make(Args&&... args){
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
    };

}

#endif //TRIDOT_REF_H
