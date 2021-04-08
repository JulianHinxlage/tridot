//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_REFLECTION_H
#define TRIDOT_REFLECTION_H

#include <string>
#include <vector>

namespace ecs{

    class Reflection{
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
            static int id = (getTypes().push_back(new TypeT<T>(getNewId())), getTypes().back()->id());
            return id;
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

    private:
        static int getNewId(){
            static int id = 0;
            return id++;
        }

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

#define ECS_UNIQUE_NAME_3(name, line, number) name##line##number
#define ECS_UNIQUE_NAME_2(name, line, number) ECS_UNIQUE_NAME_3(name, line, number)
#define ECS_UNIQUE_NAME(name) ECS_UNIQUE_NAME_2(name, __LINE__, __COUNTER__)

#define REFLECT_TYPE_NAME(type, name) bool ECS_UNIQUE_NAME(___ecs_global___) = (ecs::Reflection::registerType<type>(#name), true);
#define REFLECT_TYPE(type) REFLECT_TYPE_NAME(type, type)

#define REFLECT_MEMBER(type, member) bool ECS_UNIQUE_NAME(___ecs_global___) = (ecs::Reflection::registerMember<type, typeof(type::member)>(#member, offsetof(type, member)), true);
#define REFLECT_MEMBER2(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER(type, __VA_ARGS__)
#define REFLECT_MEMBER3(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER2(type, __VA_ARGS__)
#define REFLECT_MEMBER4(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER3(type, __VA_ARGS__)
#define REFLECT_MEMBER5(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER4(type, __VA_ARGS__)
#define REFLECT_MEMBER6(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER5(type, __VA_ARGS__)
#define REFLECT_MEMBER7(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER6(type, __VA_ARGS__)
#define REFLECT_MEMBER8(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER7(type, __VA_ARGS__)
#define REFLECT_MEMBER9(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER8(type, __VA_ARGS__)
#define REFLECT_MEMBER10(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER9(type, __VA_ARGS__)
#define REFLECT_MEMBER11(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER10(type, __VA_ARGS__)
#define REFLECT_MEMBER12(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER11(type, __VA_ARGS__)
#define REFLECT_MEMBER13(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER12(type, __VA_ARGS__)
#define REFLECT_MEMBER14(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER13(type, __VA_ARGS__)
#define REFLECT_MEMBER15(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER14(type, __VA_ARGS__)
#define REFLECT_MEMBER16(type, member, ...) REFLECT_MEMBER(type, member) REFLECT_MEMBER15(type, __VA_ARGS__)

#endif //TRIDOT_REFLECTION_H
