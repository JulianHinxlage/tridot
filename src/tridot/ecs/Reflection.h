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

namespace tridot{

    namespace impl {
        template<class T, class EqualTo>
        struct has_operator_equal_impl
        {
            template<class U, class V>
            static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
            template<typename, typename>
            static auto test(...)->std::false_type;

            using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
        };
        template<class T, class EqualTo = T>
        struct has_operator_equal : has_operator_equal_impl<T, EqualTo>::type {};
    }

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
            virtual bool equals(void* v1, void* v2) = 0;
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
        static Type *get(){
            return getTypes()[id<T>()];
        }

        static Type *get(int id){
            return getTypes()[id];
        }

        static std::vector<Type*> &getTypes(){
            static std::vector<Type*> types;
            return types;
        }

        template<typename T>
        static Type *registerType(const std::string &name){
            TypeT<T> *t = (TypeT<T>*)getTypes()[id<T>()];
            t->typeName = name;
            return t;
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
            virtual bool equals(void* v1, void* v2) override {
                if constexpr (impl::has_operator_equal<T>()) {
                    return *(T*)v1 == *(T*)v2;
                } else {
                    return false;
                }
            }
        };

    };

    namespace impl{
        template<typename T>
        class ReflectionRegisterer{
        public:
            ReflectionRegisterer(const std::string &name){
                Reflection::registerType<T>(name);
            }
            ~ReflectionRegisterer(){
                Reflection::unregisterType<T>();
            }
        };
    }

}

#define TRI_REFLECT_TYPE_NAME(type, name) static tridot::impl::ReflectionRegisterer<type> TRI_UNIQUE_NAME(___tri_global___)(#name);
#define TRI_REFLECT_TYPE(type) TRI_REFLECT_TYPE_NAME(type, type)

#define TRI_REFLECT_MEMBER(type, member) bool TRI_UNIQUE_NAME(___tri_global___) = (Reflection::registerMember<type, decltype(type::member)>(#member, offsetof(type, member)), true);
#define TRI_REFLECT_MEMBER2(type, member, member2) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2)
#define TRI_REFLECT_MEMBER3(type, member, member2, member3) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) TRI_REFLECT_MEMBER(type, member3)
#define TRI_REFLECT_MEMBER4(type, member, member2, member3, member4) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4)
#define TRI_REFLECT_MEMBER5(type, member, member2, member3, member4, member5) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4) TRI_REFLECT_MEMBER(type, member5)
#define TRI_REFLECT_MEMBER6(type, member, member2, member3, member4, member5, member6) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4) TRI_REFLECT_MEMBER(type, member5) TRI_REFLECT_MEMBER(type, member6)
#define TRI_REFLECT_MEMBER7(type, member, member2, member3, member4, member5, member6, member7) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) \
    TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4) TRI_REFLECT_MEMBER(type, member5) TRI_REFLECT_MEMBER(type, member6) TRI_REFLECT_MEMBER(type, member7)
#define TRI_REFLECT_MEMBER8(type, member, member2, member3, member4, member5, member6, member7, member8) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) \
    TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4) TRI_REFLECT_MEMBER(type, member5) TRI_REFLECT_MEMBER(type, member6) TRI_REFLECT_MEMBER(type, member7)  TRI_REFLECT_MEMBER(type, member8)
#define TRI_REFLECT_MEMBER9(type, member, member2, member3, member4, member5, member6, member7, member8, member9) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) \
    TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4) TRI_REFLECT_MEMBER(type, member5) TRI_REFLECT_MEMBER(type, member6) TRI_REFLECT_MEMBER(type, member7)  TRI_REFLECT_MEMBER(type, member8) TRI_REFLECT_MEMBER(type, member9)
#define TRI_REFLECT_MEMBER10(type, member, member2, member3, member4, member5, member6, member7, member8, member9, member10) TRI_REFLECT_MEMBER(type, member) TRI_REFLECT_MEMBER(type, member2) \
    TRI_REFLECT_MEMBER(type, member3) TRI_REFLECT_MEMBER(type, member4) TRI_REFLECT_MEMBER(type, member5) TRI_REFLECT_MEMBER(type, member6) TRI_REFLECT_MEMBER(type, member7)  TRI_REFLECT_MEMBER(type, member8) TRI_REFLECT_MEMBER(type, member9) TRI_REFLECT_MEMBER(type, member10)


#endif //TRIDOT_REFLECTION_H
