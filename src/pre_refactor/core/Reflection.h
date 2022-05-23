//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"
#include "util/Ref.h"

namespace tri {

    namespace impl {
        template<class T, class U, class> struct has_equal_impl : std::false_type {};
        template<class T, class U> struct has_equal_impl<T, U, decltype((bool)(std::declval<T>() == std::declval<U>()), void())> : std::true_type {};
        template<class T, class U> struct has_equal : has_equal_impl<T, U, void> {};

        template <typename T> struct is_vector : std::false_type { };
        template <typename T, typename Alloc> struct is_vector<std::vector<T, Alloc>> : std::true_type {};


#define define_has_member(name)                                                    \
        template <typename T>                                                      \
        class has_member_##name                                                    \
        {                                                                          \
            typedef char yes_type;                                                 \
            typedef long no_type;                                                  \
            template <typename U> static yes_type test(decltype(&U::name));        \
            template <typename U> static no_type  test(...);                       \
        public:                                                                    \
            static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);  \
        };
#define has_member(type, name)  tri::impl::has_member_##name<type>::value

        define_has_member(update);
        define_has_member(startup);
        define_has_member(shutdown);
        define_has_member(init);
    }

    class Reflection : public System {
    public:
        class TypeDescriptor;

        enum TypeFlags {
            NONE = 0,
            COMPONENT = 1,
            ASSET = 2,
            NOT_SERIALIZED = 4,
            HIDDEN_IN_EDITOR = 8,
            REFERENCE = 16,
            VECTOR = 32,
        };

        class MemberDescriptor {
        public:
            std::string name;
            int offset;
            TypeFlags flags;
            TypeDescriptor* type;
            void *minValue;
            void *maxValue;
        };

        class ConstantDescriptor {
        public:
            std::string name;
            int value;
        };

        class TypeDescriptor {
        public:
            std::string name;
            std::string group;
            int size;
            TypeFlags flags;
            std::vector<MemberDescriptor> member;
            std::vector<ConstantDescriptor> constants;
            int typeId;
            TypeDescriptor* baseType;


            template<typename T>
            bool isType() const {
                return hashCode == typeid(T).hash_code();
            }

            virtual void construct(void* ptr) const = 0;
            virtual void destruct(void* ptr) const = 0;
            virtual void* alloc() const = 0;
            virtual void free(void* ptr) const = 0;
            virtual void copy(const void* from, void* to) const = 0;
            virtual bool equals(void* v1, void* v2) const = 0;
            virtual bool hasEquals() const = 0;
            virtual void swap(void* v1, void* v2) const = 0;
            virtual void move(void* from, void* to, int count) const = 0;
            virtual void construct(void* ptr, int count) const = 0;
            virtual void destruct(void* ptr, int count) const = 0;
            virtual void copy(void* from, void* to, int count) const = 0;

            virtual int vectorSize(void *ptr) const { return 0; }
            virtual void* vectorGet(void* ptr, int index) const { return nullptr; }
            virtual void vectorInsert(void* ptr, int index, void* elementPtr) const {}
            virtual void vectorErase(void* ptr, int index) const {}
            virtual void vectorClear(void* ptr) const {}

        private:
            friend class Reflection;
            size_t hashCode;
            bool initFlag = false;
        };

        template<typename T>
        int getTypeId() {
            static int typeId = getNewTypeId<T>();
            return typeId;
        }

        const TypeDescriptor* getType(int typeId) {
            if (typeId >= 0 && typeId < descriptors.size()) {
                return descriptors[typeId].get();
            }
            else {
                return nullptr;
            }
        }

        template<typename T>
        const TypeDescriptor* getType() {
            return getType(getTypeId<T>());
        }

        const TypeDescriptor* getType(const std::string &name) {
            auto iter = descriptorsByName.find(name);
            if(iter != descriptorsByName.end()) {
                return iter->second;
            }
            return nullptr;
        }

        template<typename T>
        int registerType(const std::string &name, const std::string &group = "", TypeFlags flags = NONE) {
            int typeId = registerTypeImpl<T>(name, group, flags);
            if ((flags & ASSET) && !(flags & REFERENCE)) {
                int refTypeId = registerTypeImpl<Ref<T>>("Ref<" + name +  ">", group, (TypeFlags)(flags | REFERENCE));
                descriptors[refTypeId]->baseType = descriptors[typeId].get();
            }
            return typeId;
        }

        template<typename T>
        void registerMember(int typeId, const std::string& name, int offset, TypeFlags flags = NONE, T min = T(), T max = T()) {
            if (typeId >= 0 && typeId < descriptors.size()) {
                TypeDescriptor* desc = descriptors[typeId].get();
                TypeDescriptor* memberDesc = descriptors[getTypeId<T>()].get();
                for (auto& m : desc->member) {
                    if (m.name == name) {
                        return;
                    }
                }
                MemberDescriptor& member = desc->member.emplace_back();
                member.name = name;
                member.offset = offset;
                member.type = memberDesc;
                member.flags = flags;

                member.minValue = nullptr;
                member.maxValue = nullptr;
                if constexpr (!impl::is_vector<T>::value) {
                    if constexpr (std::is_copy_constructible<T>::value) {
                        if(!(min == T() && max == T())) {
                            member.minValue = new T(min);
                            member.maxValue = new T(max);
                        }
                    }
                }
            }
        }

        template<typename T, typename MemberType>
        void registerMember(const std::string& name, int offset, TypeFlags flags = NONE, MemberType min = MemberType(), MemberType max = MemberType()) {
            registerMember<MemberType>(getTypeId<T>(), name, offset, flags, min, max);
        }

        void registerConstant(int typeId, const std::string& name, int value) {
            if (typeId >= 0 && typeId < descriptors.size()) {
                TypeDescriptor* desc = descriptors[typeId].get();
                for (auto& c : desc->constants) {
                    if (c.name == name) {
                        return;
                    }
                }
                desc->constants.push_back({name, value});
            }
        }

        template<typename T>
        void registerConstant(const std::string& name, int offset) {
            registerConstant(getTypeId<T>(), name, offset);
        }

        void unregisterType(int typeId);

        template<typename T>
        void unregisterType() {
            unregisterType(getTypeId<T>());
        }

        const std::vector<std::shared_ptr<TypeDescriptor>>& getDescriptors() {
            return descriptors;
        }

    private:
        std::vector<std::shared_ptr<TypeDescriptor>> descriptors;
        std::unordered_map<std::string, TypeDescriptor*> descriptorsByName;

        template<typename T>
        int getNewTypeId() {
            //check if the type allready has an assigned typeId
            size_t hashCode = typeid(T).hash_code();
            for (auto &desc : descriptors) {
                if (desc) {
                    if (desc->hashCode == hashCode) {
                        return desc->typeId;
                    }
                }
            }

            //check if a typeId can be reused
            int typeId = (int)descriptors.size();
            for (int i = 0; i < descriptors.size(); i++) {
                auto& desc = descriptors[i];
                if (descriptors[i] == nullptr) {
                    typeId = i;
                    break;
                }
            }

            std::shared_ptr<TypeDescriptor> desc;
            if constexpr (impl::is_vector<T>::value) {
                //create new vector descriptor
                desc = std::make_shared<VectorDescriptorT<T::value_type>>();
                desc->baseType = descriptors[getTypeId<T::value_type>()].get();
                desc->flags = VECTOR;
            }
            else {
                //create new descriptor
                desc = std::make_shared<TypeDescriptorT<T>>();
                desc->baseType = nullptr;
                desc->flags = NONE;
            }

            //determin final typeId
            while (typeId >= descriptors.size() || descriptors[typeId] != nullptr) {
               typeId = descriptors.size();
               descriptors.push_back(nullptr);
            }

            desc->hashCode = hashCode;
            desc->typeId = typeId;
            desc->size = sizeof(T);
            desc->name = typeid(T).name();
            descriptorsByName[desc->name] = desc.get();
            descriptors[typeId] = desc;

            return typeId;
        }

        template<typename T>
        int registerTypeImpl(const std::string& name, const std::string& group = "", TypeFlags flags = NONE) {
            int typeId = getTypeId<T>();
            TypeDescriptor* desc = descriptors[typeId].get();
            descriptorsByName.erase(desc->name);
            desc->name = name;
            desc->group = group;
            desc->flags = (TypeFlags)(desc->flags | flags);
            desc->hashCode = typeid(T).hash_code();
            descriptorsByName[desc->name] = desc;
            return typeId;
        }

        void update() override;

        template<typename T>
        class TypeDescriptorT : public TypeDescriptor {
        public:
            void construct(void* ptr) const override {
                if constexpr (std::is_constructible<T>::value) {
                    new ((T*)ptr) T();
                }
            }
            void destruct(void* ptr) const override {
                if constexpr (std::is_destructible<T>::value) {
                    ((T*)ptr)->~T();
                }
            }
            void* alloc() const override {
                return new T();
            }
            void free(void* ptr) const override {
                delete (T*)ptr;
            }
            void copy(const void* from, void* to) const override {
                if constexpr (std::is_copy_constructible<T>::value) {
                    new ((T*)to) T(*(T*)from);
                }
            }
            bool equals(void* v1, void* v2) const override {
                if constexpr (impl::is_vector<T>::value) {
                    if constexpr (impl::has_equal<T, T>()) {
                        if constexpr (impl::has_equal<T::value_type, T::value_type>()) {
                            return *(T*)v1 == *(T*)v2;
                        }
                    }
                }
                else {
                    if constexpr (impl::has_equal<T, T>()) {
                        return *(T*)v1 == *(T*)v2;
                    }
                }
                return false;
            }
            bool hasEquals() const override {
                if constexpr (impl::is_vector<T>::value) {
                    if constexpr (impl::has_equal<T, T>()) {
                        if constexpr (impl::has_equal<T::value_type, T::value_type>()) {
                            return true;
                        }
                    }
                }
                else {
                    if constexpr (impl::has_equal<T, T>()) {
                        return true;
                    }
                }
                return false;
            }
            void swap(void* v1, void* v2) const override {
                std::swap(*(T*)v1, *(T*)v2);
            }
            void move(void* from, void* to, int count) const override {
                if constexpr (std::is_move_constructible<T>::value) {
                    T *f = (T*)from;
                    T *t = (T*)to;
                    for(int i = 0; i < count; i++) {
                        new (t) T(std::move(*f));
                        ++f;
                        ++t;
                    }
                }else {
                    copy(from, to, count);
                }
            }
            void construct(void* ptr, int count) const override {
                if constexpr (std::is_constructible<T>::value) {
                    T *p = (T*)ptr;
                    for(int i = 0; i < count; i++) {
                        new (p) T();
                        p++;
                    }
                }
            }
            void destruct(void* ptr, int count) const override {
                if constexpr (std::is_destructible<T>::value) {
                    T *p = (T*)ptr;
                    for(int i = 0; i < count; i++) {
                        p->~T();
                        p++;
                    }
                }
            }
            void copy(void* from, void* to, int count) const override {
                if constexpr (std::is_copy_constructible<T>::value) {
                    T *f = (T*)from;
                    T *t = (T*)to;
                    for(int i = 0; i < count; i++) {
                        new (t) T(*f);
                        ++f;
                        ++t;
                    }
                }
            }
        };
        
        template<typename E>
        class VectorDescriptorT : public TypeDescriptor {
        public:
            using T = std::vector<E>;

            void construct(void* ptr) const override {
                if constexpr (std::is_constructible<T>::value) {
                    new ((T*)ptr) T();
                }
            }
            void destruct(void* ptr) const override {
                if constexpr (std::is_destructible<T>::value) {
                    ((T*)ptr)->~T();
                }
            }
            void* alloc() const override {
                return new T();
            }
            void free(void* ptr) const override {
                delete (T*)ptr;
            }
            void copy(const void* from, void* to) const override {
                if constexpr (std::is_copy_constructible<T>::value) {
                    new ((T*)to) T(*(T*)from);
                }
            }
            bool equals(void* v1, void* v2) const override {
                return false;
            }
            bool hasEquals() const override {
                return false;
            }
            void swap(void* v1, void* v2) const override {
                std::swap(*(T*)v1, *(T*)v2);
            }
            void move(void* from, void* to, int count) const override {
                if constexpr (std::is_move_constructible<T>::value) {
                    T* f = (T*)from;
                    T* t = (T*)to;
                    for (int i = 0; i < count; i++) {
                        new (t) T(std::move(*f));
                        ++f;
                        ++t;
                    }
                }
                else {
                    copy(from, to, count);
                }
            }
            void construct(void* ptr, int count) const override {
                if constexpr (std::is_constructible<T>::value) {
                    T* p = (T*)ptr;
                    for (int i = 0; i < count; i++) {
                        new (p) T();
                        p++;
                    }
                }
            }
            void destruct(void* ptr, int count) const override {
                if constexpr (std::is_destructible<T>::value) {
                    T* p = (T*)ptr;
                    for (int i = 0; i < count; i++) {
                        p->~T();
                        p++;
                    }
                }
            }
            void copy(void* from, void* to, int count) const override {
                if constexpr (std::is_copy_constructible<T>::value) {
                    T* f = (T*)from;
                    T* t = (T*)to;
                    for (int i = 0; i < count; i++) {
                        new (t) T(*f);
                        ++f;
                        ++t;
                    }
                }
            }

            virtual int vectorSize(void* ptr) const {
                return ((T*)ptr)->size();
            }
            virtual void* vectorGet(void* ptr, int index) const {
                return &(E&)(*(((T*)ptr)->begin() + index));
            }
            virtual void vectorInsert(void* ptr, int index, void* elementPtr) const {
                if (elementPtr == nullptr) {
                    ((T*)ptr)->insert(((T*)ptr)->begin() + index, E());
                }
                else {
                    ((T*)ptr)->insert(((T*)ptr)->begin() + index, *(E*)elementPtr);
                }
            }
            virtual void vectorErase(void* ptr, int index) const {
                ((T*)ptr)->erase(((T*)ptr)->begin() + index);
            }
            virtual void vectorClear(void* ptr) const {
                return ((T*)ptr)->clear();
            }
        };
        
    };

}