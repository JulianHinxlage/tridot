//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"

namespace tri {

    namespace impl {
        template<class T, class U, class> struct has_equal_impl : std::false_type {};
        template<class T, class U> struct has_equal_impl<T, U, decltype((bool)(std::declval<T>() == std::declval<U>()), void())> : std::true_type {};
        template<class T, class U> struct has_equal : has_equal_impl<T, U, void> {};

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
#define has_member(type, name)  tridot::impl::has_member_##name<type>::value

        define_has_member(update);
        define_has_member(startup);
        define_has_member(shutdown);
        define_has_member(init);
    }

    class Reflection : public System {
    public:
        class TypeDescriptor;

        class MemberDescriptor {
        public:
            std::string name;
            int offset;
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
            int size;
            std::vector<MemberDescriptor> member;
            std::vector<ConstantDescriptor> constants;
            int typeId;
            bool isComponent;

            template<typename T>
            bool isType() const {
                return hashCode == typeid(T).hash_code();
            }

            virtual void construct(void* ptr) const = 0;
            virtual void destruct(void* ptr) const = 0;
            virtual void* alloc() const = 0;
            virtual void free(void* ptr) const = 0;
            virtual void copy(void* from, void* to) const = 0;
            virtual bool equals(void* v1, void* v2) const = 0;
        private:
            friend class Reflection;
            int hashCode;
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

        template<typename T>
        int registerType(const std::string &name, bool isComponent = false) {
            TypeDescriptor* desc = descriptors[getTypeId<T>()].get();
            desc->name = name;
            desc->isComponent = isComponent;
            return desc->typeId;
        }

        template<typename T>
        void registerMember(int typeId, const std::string& name, int offset, T min = T(), T max = T()) {
            if (typeId >= 0 && typeId < descriptors.size()) {
                TypeDescriptor* desc = descriptors[typeId].get();
                TypeDescriptor* memberDesc = descriptors[getTypeId<T>()].get();
                for (auto& m : desc->member) {
                    if (m.name == name) {
                        return;
                    }
                }
                if(min == T() && max == T()) {
                    desc->member.push_back({name, offset, memberDesc, nullptr, nullptr});
                }else {
                    desc->member.push_back({name, offset, memberDesc, new T(min), new T(max)});
                }
            }
        }

        template<typename T, typename MemberType>
        void registerMember(const std::string& name, int offset, MemberType min = MemberType(), MemberType max = MemberType()) {
            registerMember<MemberType>(getTypeId<T>(), name, offset, min, max);
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

        void unregisterType(int typeId) {
            if (typeId >= 0 && typeId < descriptors.size()) {
                descriptors[typeId] = nullptr;
            }
        }

        template<typename T>
        void unregisterType() {
            unregisterType(getTypeId<T>());
        }

        const std::vector<std::shared_ptr<TypeDescriptor>>& getDescriptors() {
            return descriptors;
        }

    private:
        std::vector<std::shared_ptr<TypeDescriptor>> descriptors;

        template<typename T>
        int getNewTypeId() {
            int hashCode = (int)typeid(T).hash_code();
            for (auto &desc : descriptors) {
                if (desc) {
                    if (desc->hashCode == hashCode) {
                        return desc->typeId;
                    }
                }
            }
            std::shared_ptr<TypeDescriptorT<T>> desc = std::make_shared<TypeDescriptorT<T>>();
            desc->hashCode = hashCode;
            desc->typeId = (int)descriptors.size();
            desc->size = sizeof(T);
            desc->name = typeid(T).name();
            descriptors.push_back(desc);
            return desc->typeId;
        }

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
            virtual void* alloc() const override {
                return new T();
            }
            virtual void free(void* ptr) const override {
                delete (T*)ptr;
            }
            void copy(void* from, void* to) const override {
                if constexpr (std::is_copy_constructible<T>::value) {
                    new ((T*)to) T(*(T*)from);
                }
            }
            virtual bool equals(void* v1, void* v2) const override {
                if constexpr (impl::has_equal<T, T>()) {
                    return *(T*)v1 == *(T*)v2;
                }
                else {
                    return false;
                }
            }
        };

    };

}