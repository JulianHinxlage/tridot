//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TYPEMAP_H
#define TRIDOT_TYPEMAP_H

#include <memory>
#include <vector>
#include <istream>
#include <ostream>

namespace ecs{

    template<typename T>
    void serialize(std::ostream &stream, const T &t){
        stream.write((const char*)&t, sizeof(T));
    }

    template<typename T>
    void deserialize(std::istream &stream, T &t){
        stream.read((char*)&t, sizeof(T));
    }

    class TypeMap{
    public:
        class Type{
        public:
            virtual const std::type_info &info() = 0;
            virtual int size() = 0;
            virtual void destruct(void *ptr) = 0;
            virtual void construct(void *ptr) = 0;
            virtual void copy(void *from, void *to) = 0;
            virtual void serialize(std::ostream &stream, const void *ptr) = 0;
            virtual void deserialize(std::istream &stream, void *ptr) = 0;
        };

        template<typename T>
        int id(){
            int sid = staticId<T>();
            while(sid >= map.size()){
                map.push_back(-1);
            }
            if(map[sid] == -1){
                map[sid] = types.size();
                types.push_back(staticTypes()[sid]);
            }
            return map[sid];
        }

        int id(int staticId){
            if(staticId >= map.size()){
                return -1;
            }
            return map[staticId];
        }

        Type &get(int id){
            return *(types[id].get());
        }

        int size(){
            return types.size();
        }

        template<typename T>
        static int staticId() {
            static int id{(staticTypes().push_back(std::make_shared<TypeT<T>>()), newStaticId())};
            return id;
        }

        static Type &staticGet(int id){
            return *staticTypes()[id].get();
        }

        static int staticSize(){
            return staticTypes().size();
        }

    private:
        template<typename T>
        class TypeT : public Type{
        public:
            const std::type_info &info() override{
                return typeid(T);
            }

            int size() override{
                return sizeof(T);
            }

            void destruct(void *ptr) override{
                if constexpr(std::is_destructible<T>::value) {
                    ((T*)ptr)->~T();
                }
            }

            void construct(void *ptr) override{
                if constexpr(std::is_constructible<T>::value){
                    new ((T*)ptr) T();
                }
            }

            void copy(void *from, void *to) override{
                if constexpr(std::is_copy_constructible<T>::value){
                    new ((T*)to) T(*(T*)from);
                }
            }

            void serialize(std::ostream &stream, const void *ptr) override{
                ecs::serialize<T>(stream, *(T*)ptr);
            }

            void deserialize(std::istream &stream, void *ptr) override{
                ecs::deserialize<T>(stream, *(T*)ptr);
            }
        };

        static int newStaticId(){
            static int id{0};
            return id++;
        }

        std::vector<int> map;
        std::vector<std::shared_ptr<Type>> types;

        static std::vector<std::shared_ptr<Type>> &staticTypes(){
            static std::vector<std::shared_ptr<Type>> types;
            return types;
        }
    };

}

#endif //TRIDOT_TYPEMAP_H
