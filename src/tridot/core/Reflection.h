//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

namespace tridot {

    namespace impl{
        template<class T, class U, class> struct has_equal_impl                                                                 : std::false_type {};
        template<class T, class U>        struct has_equal_impl<T, U, decltype(std::declval<T>() == std::declval<U>(), void())> : std::true_type {};
        template<class T, class U> struct has_equal : has_equal_impl<T, U, void> {};
    }

    class Reflection {
    public:
        class TypeDescriptor;

        class Member{
        public:
            std::string name;
            int offset;
            TypeDescriptor *descriptor;
        };

        class Constant{
        public:
            std::string name;
            int value;
        };

        class TypeDescriptor{
        public:
            virtual ~TypeDescriptor(){}

            int id(){return typeId;}
            int size(){return typeSize;}
            size_t hashCode(){return typeHashCode;}
            const std::string &name(){return typeName;}
            const std::vector<Member> &member(){return typeMember;}
            const std::vector<Constant> &constants(){return typeConstants;}

            template<typename T>
            bool isType(){
                return typeHashCode == typeid(T).hash_code();
            }

            virtual void construct(void *ptr) = 0;
            virtual void destruct(void *ptr) = 0;
            virtual void *alloc() = 0;
            virtual void free(void *ptr) = 0;
            virtual void copy(void *from, void *to) = 0;
            virtual bool equals(void* v1, void* v2) = 0;

        protected:
            friend class Reflection;
            int typeId;
            int typeSize;
            size_t typeHashCode;
            std::string typeName;
            std::vector<Member> typeMember;
            std::vector<Constant> typeConstants;
        };

        template<typename Type>
        int getTypeId(){
            static int typeId = [this](){
                size_t hashCode = typeid(Type).hash_code();
                for(auto &descriptor : descriptors){
                    if(descriptor){
                        if(descriptor->hashCode() == hashCode){
                            return descriptor->typeId;
                        }
                    }
                }
                int id = descriptors.size();
                auto *descriptor = new TypeDescriptorT<Type>();
                descriptors.push_back(descriptor);
                descriptor->typeHashCode = hashCode;
                descriptor->typeId = id;
                descriptor->typeName = typeid(Type).name();
                descriptor->typeSize = sizeof(Type);
                return id;
            }();
            return typeId;
        }

        template<typename Type>
        TypeDescriptor *getDescriptor(){
            return descriptors[getTypeId<Type>()];
        }

        TypeDescriptor *getDescriptor(int typeId){
            if(typeId >= 0 && typeId < descriptors.size()){
                return descriptors[typeId];
            }else{
                return nullptr;
            }
        }

        const std::vector<TypeDescriptor*> &getDescriptors(){
            return descriptors;
        }

        template<typename Type>
        TypeDescriptor *addType(const std::string &name){
            auto *descriptor = getDescriptor<Type>();
            descriptor->typeName = name;
            return descriptor;
        }

        void removeType(int typeId){
            if(typeId >= 0 && typeId < descriptors.size()){
                delete descriptors[typeId];
                descriptors[typeId] = nullptr;
            }
        }

        template<typename Type>
        void removeType(){
            removeType(getTypeId<Type>());
        }

        template<typename MemberType>
        void addMember(TypeDescriptor *descriptor, const std::string &name, int offset){
            if(descriptor){
                for(auto &member : descriptor->member()){
                    if(member.name == name){
                        return;
                    }
                }

                Member member;
                member.name = name;
                member.offset = offset;
                member.descriptor = getDescriptor<MemberType>();
                descriptor->typeMember.push_back(member);
            }
        }

        template<typename Type, typename MemberType>
        void addMember(const std::string &name, int offset){
            addMember<MemberType>(getDescriptor<Type>(), name, offset);
        }

        void addConstant(TypeDescriptor *descriptor, const std::string &name, int value){
            if(descriptor){
                for(auto &constant : descriptor->constants()){
                    if(constant.name == name){
                        return;
                    }
                }

                Constant constant;
                constant.name = name;
                constant.value = value;
                descriptor->typeConstants.push_back(constant);
            }
        }

        template<typename Type>
        void addConstant(const std::string &name, int value){
            addConstant(getDescriptor<Type>(), name, value);
        }

    private:
        template<typename T>
        class TypeDescriptorT : public TypeDescriptor{
        public:
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
            virtual void *alloc() override{
                return new T();
            }
            virtual void free(void *ptr) override{
                delete (T*)ptr;
            }
            void copy(void *from, void *to) override{
                if constexpr(std::is_copy_constructible<T>::value){
                    new ((T*)to) T(*(T*)from);
                }
            }
            virtual bool equals(void* v1, void* v2) override {
                if constexpr (impl::has_equal<T, T>()) {
                    return *(T*)v1 == *(T*)v2;
                } else {
                    return false;
                }
            }
        };

        std::vector<TypeDescriptor*> descriptors;
    };

}
