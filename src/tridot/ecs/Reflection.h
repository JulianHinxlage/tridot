//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_REFLECTION_H
#define TRIDOT_REFLECTION_H

#include "tridot/core/config.h"
#include <string>
#include <vector>
#include <typeinfo>
#include <map>

namespace ecs{

    class TRI_API Reflection{
    public:
        class Member{
        public:
            std::string name;
            int typeId;
            int offset;
        };

        class Type{
        public:
            virtual int id() = 0;
            virtual int size() = 0;
            virtual const std::string &name() = 0;
            virtual const std::type_info &info() = 0;
            virtual std::vector<Member> &member() = 0;
            virtual void construct(void *ptr) = 0;
            virtual void destruct(void *ptr) = 0;
            virtual void copy(void *from, void *to) = 0;
        };

        template<typename T>
        static int id(){
            static bool init = false;
            static int id = 0;
            if(!init){
                init = true;
                auto& types = getTypes();
                int hash = typeid(T).hash_code();
                for (auto* t : types) {
                    if (t) {
                        if ((int)t->info().hash_code() == hash) {
                            id = t->id();
                            return id;
                        }
                    }
                }
                for (int i = 0; i < types.size(); i++) {
                    if (types[i] == nullptr) {
                        types[i] = new TypeT<T>(i);
                        id = i;
                        return id;
                    }
                }
                id = types.size();
                types.push_back(new TypeT<T>(id));
                return id;
            } else {
                return id;
            }
        }

        template<class T>
        static Type &get(){
            return *getTypes()[id<T>()];
        }

        static Type &get(int id){
            return *getTypes()[id];
        }

        static std::vector<Type*> &getTypes(){
            static std::vector<Type*> types;
            return types;
        }

        template<typename T>
        static Type &registerType(const std::string &name){
            TypeT<T> *t = (TypeT<T>*)getTypes()[id<T>()];
            t->typeName = name;
            return *t;
        }

        template<typename T, typename M>
        static void registerMember(const std::string &name, int offset){
            TypeT<T> *t = (TypeT<T>*)getTypes()[id<T>()];
            for(auto &m : t->typeMember){
                if(m.name == name){
                    return;
                }
            }
            t->typeMember.push_back({name, id<M>(), offset});
        }

        template<typename T>
        static void unregisterType(){
            int i = id<T>();
            TypeT<T> *t = (TypeT<T>*)getTypes()[i];
            delete t;
            getTypes()[i] = nullptr;
        }

    private:
        template<typename T>
        class TypeT : public Type{
        public:
            int typeId;
            std::string typeName;
            std::vector<Member> typeMember;

            TypeT(int id) : typeId(id){
                typeName = typeid(T).name();
            }

            virtual int id() override{
                return typeId;
            }
            virtual int size() override{
                return sizeof(T);
            }
            virtual const std::type_info &info() override{
                return typeid(T);
            }
            virtual const std::string &name() override{
                return typeName;
            }
            virtual std::vector<Member> &member() override{
                return typeMember;
            }

            void construct(void *ptr) override{
                if constexpr(std::is_constructible<T>::value){
                    new ((T*)ptr) T();
                }
            }
            void destruct(void *ptr) override{
                if constexpr(std::is_destructible<T>::value) {
                    ((T*)ptr)->~T();
                }
            }
            void copy(void *from, void *to) override{
                if constexpr(std::is_copy_constructible<T>::value){
                    new ((T*)to) T(*(T*)from);
                }
            }
        };

    };

}

template<typename T>
class ReflectionRegisterer{
public:
    ReflectionRegisterer(const std::string &name){
        ecs::Reflection::registerType<T>(name);
    }
    ~ReflectionRegisterer(){
        ecs::Reflection::unregisterType<T>();
    }
};

#define REFLECT_TYPE_NAME(type, name) ReflectionRegisterer<type> ECS_UNIQUE_NAME(___ecs_global___)(#name);
#define REFLECT_TYPE(type) REFLECT_TYPE_NAME(type, type)

#define REFLECT_MEMBER(type, member) bool ECS_UNIQUE_NAME(___ecs_global___) = (ecs::Reflection::registerMember<type, decltype(type::member)>(#member, offsetof(type, member)), true);
#define REFLECT_MEMBER2(type, member, member2) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2)
#define REFLECT_MEMBER3(type, member, member2, member3) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) REFLECT_MEMBER(type, member3)
#define REFLECT_MEMBER4(type, member, member2, member3, member4) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4)
#define REFLECT_MEMBER5(type, member, member2, member3, member4, member5) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4) REFLECT_MEMBER(type, member5)
#define REFLECT_MEMBER6(type, member, member2, member3, member4, member5, member6) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4) REFLECT_MEMBER(type, member5) REFLECT_MEMBER(type, member6)
#define REFLECT_MEMBER7(type, member, member2, member3, member4, member5, member6, member7) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) \
    REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4) REFLECT_MEMBER(type, member5) REFLECT_MEMBER(type, member6) REFLECT_MEMBER(type, member7)
#define REFLECT_MEMBER8(type, member, member2, member3, member4, member5, member6, member7, member8) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) \
    REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4) REFLECT_MEMBER(type, member5) REFLECT_MEMBER(type, member6) REFLECT_MEMBER(type, member7)  REFLECT_MEMBER(type, member8)
#define REFLECT_MEMBER9(type, member, member2, member3, member4, member5, member6, member7, member8, member9) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) \
    REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4) REFLECT_MEMBER(type, member5) REFLECT_MEMBER(type, member6) REFLECT_MEMBER(type, member7)  REFLECT_MEMBER(type, member8) REFLECT_MEMBER(type, member9)
#define REFLECT_MEMBER10(type, member, member2, member3, member4, member5, member6, member7, member8, member9, member10) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, member2) \
    REFLECT_MEMBER(type, member3) REFLECT_MEMBER(type, member4) REFLECT_MEMBER(type, member5) REFLECT_MEMBER(type, member6) REFLECT_MEMBER(type, member7)  REFLECT_MEMBER(type, member8) REFLECT_MEMBER(type, member9) REFLECT_MEMBER(type, member10)


#endif //TRIDOT_REFLECTION_H
