//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "util/Ref.h"

namespace tri {

	class ClassDescriptor;

	class PropertyDescriptor {
	public:
		enum Flags {
			NONE = 0,
			HIDDEN = 1 << 0,
			NO_SERIALIZE = 1 << 1,
		};

		std::string name;
		ClassDescriptor* type;
		int offset;
		void* min;
		void* max;
		Flags flags;
	};

	class FunctionDescriptor {
	public:
		std::string name;
		ClassDescriptor* returnType;
		std::vector<ClassDescriptor*> parameterTypes;

		virtual void invoke(void* object) const = 0;
	};

	class ClassDescriptor {
	public:
		enum Flags {
			NONE = 0,
			HIDDEN = 1 << 0,
			NO_SERIALIZE = 1 << 1,
			COMPONENT = 1 << 2,
			ASSET = 1 << 3,
			SYSTEM = 1 << 4,
			REFERENCE = 1 << 5,
			VECTOR = 1 << 6,
		};

		int classId;
		std::string name;
		std::string category;
		int size;
		size_t hashCode;
		Flags flags;
		std::vector<PropertyDescriptor> properties;
		std::vector<FunctionDescriptor*> functions;
		std::vector<std::pair<std::string, int>> enumValues;
		ClassDescriptor* elementType;
		void* registrationSourceAddress;

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
		virtual void copy(const void* from, void* to, int count) const = 0;

		virtual int vectorSize(void* ptr) const { return 0; }
		virtual void* vectorGet(void* ptr, int index) const { return nullptr; }
		virtual void vectorInsert(void* ptr, int index, void* elementPtr) const {}
		virtual void vectorErase(void* ptr, int index) const {}
		virtual void vectorClear(void* ptr) const {}

	private:
		friend class Reflection;
		bool wasRegisteredExplicit = false;
		bool wasRegisterCallbackInvoked = false;
	};

	namespace impl {
		template<class T, class U, class> struct has_equal_impl : std::false_type {};
		template<class T, class U> struct has_equal_impl<T, U, decltype((bool)(std::declval<T>() == std::declval<U>()), void())> : std::true_type {};
		template<class T, class U> struct has_equal : has_equal_impl<T, U, void> {};

		template <typename T> struct is_vector : std::false_type { };
		template <typename T, typename Alloc> struct is_vector<std::vector<T, Alloc>> : std::true_type {};
	}

	class Reflection {
	public:
		static const std::vector<ClassDescriptor*>& getDescriptors() {
			return getDescriptorsImpl();
		}

		static const ClassDescriptor* getDescriptor(int classId) {
			return getDescriptorsImpl()[classId];
		}
		
		static const ClassDescriptor* getDescriptor(const std::string &name) {
			auto entry = getDescriptorsByNameImpl().find(name);
			if (entry != getDescriptorsByNameImpl().end()) {
				return entry->second;
			}
			else {
				return nullptr;
			}
		}

		template<typename ClassType>
		static const ClassDescriptor* getDescriptor() {
			return getDescriptorsImpl()[getClassId<ClassType>()];
		}
		
		template<typename ClassType>
		static int getClassId() {
			static int classId = registerClassId<ClassType>(false);
			return classId;
		}

		template<typename ClassType>
		static void registerClass(const std::string &name, ClassDescriptor::Flags flags = ClassDescriptor::NONE, const std::string& category = "") {
			registerClassImpl<ClassType>(name, flags, category);
			if (flags & ClassDescriptor::ASSET) {
				if (!(flags & ClassDescriptor::REFERENCE)) {
					registerClassImpl<Ref<ClassType>>(std::string("Ref<") + name + ">",  (ClassDescriptor::Flags)(flags | ClassDescriptor::REFERENCE), category);
					getDescriptorsImpl()[getClassId<Ref<ClassType>>()]->elementType = getDescriptorsImpl()[getClassId<ClassType>()];
				}
			}
		}

		template<typename ClassType, typename PropertyType>
		static void registerProperty(const std::string& name, int offset, PropertyDescriptor::Flags flags = PropertyDescriptor::NONE) {
			auto* desc = getDescriptorsImpl()[getClassId<ClassType>()];

			for (auto &prop : desc->properties) {
				if (prop.name == name) {
					return;
				}
			}

			PropertyDescriptor prop;
			prop.name = name;
			prop.type = getDescriptorsImpl()[getClassId<PropertyType>()];
			prop.offset = offset;
			prop.flags = flags;
			prop.min = nullptr;
			prop.max = nullptr;
			desc->properties.push_back(prop);
		}

		template<typename ClassType, typename PropertyType>
		static void registerPropertyRange(const std::string& name, PropertyType min, PropertyType max) {
			auto* desc = getDescriptorsImpl()[getClassId<ClassType>()];
			for (auto& prop : desc->properties) {
				if (prop.name == name) {
					prop.min = new PropertyType(min);
					prop.max = new PropertyType(max);
					return;
				}
			}
		}

		template<typename ClassType, typename PropertyType>
		static void registerProperty(const std::string& name, int offset, PropertyDescriptor::Flags flags, PropertyType min, PropertyType max) {
			registerProperty<ClassType, PropertyType>(name, offset, flags);
			registerPropertyRange<ClassType, PropertyType>(name, min, max);
		}

		template<typename ClassType>
		static void registerEnumValue(const std::string& name, int value) {
			auto* desc = getDescriptorsImpl()[getClassId<ClassType>()];
			desc->enumValues.push_back({ name, value });
		}

		template<typename ClassType>
		static void registerFunction(const std::string& name, void (ClassType::*func)()) {
			auto* desc = getDescriptorsImpl()[getClassId<ClassType>()];
			auto fdesc = new FunctionDescriptorT<ClassType>();
			fdesc->function = func;
			fdesc->name = name;
			desc->functions.push_back(fdesc);
		}

		template<typename ClassType>
		static FunctionDescriptor* getFunctionDescriptor(void (ClassType::* func)()) {
			auto* desc = getDescriptorsImpl()[getClassId<ClassType>()];
			for (auto* fdesc : desc->functions) {
				if (fdesc) {
					if (((FunctionDescriptorT<ClassType>*)fdesc)->function == func) {
						return fdesc;
					}
				}
			}
			return nullptr;
		}

		template<typename ClassType>
		static void unregisterClass() {
			unregisterClass(getClassId<ClassType>());
		}

		static void unregisterClass(int classId) {
			auto *desc = getDescriptorsImpl()[classId];
			if (desc) {
				getDescriptorsByNameImpl().erase(desc->name);
				onClassUnregister(desc->classId);
				for (auto& fdesc : desc->functions) {
					delete fdesc;
				}
				delete desc;
			}
			getDescriptorsImpl()[classId] = nullptr;
		}

	private:

		template<typename T>
		class ClassDescriptorT : public ClassDescriptor {
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
				if constexpr (std::is_copy_constructible<T>::value) {
					std::swap(*(T*)v1, *(T*)v2);
				}
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
			void copy(const void* from, void* to, int count) const override {
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
		};

		template<typename E>
		class ClassDescriptorVector : public ClassDescriptorT<std::vector<E>> {
		public:
			using T = std::vector<E>;

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

		template<typename T>
		class FunctionDescriptorT : public FunctionDescriptor {
		public:
			void (T::* function)();

			virtual void invoke(void* object) const {
				((T*)object->*function)();
			}
		};

		static std::vector<ClassDescriptor*>& getDescriptorsImpl();
		static std::map<std::string, ClassDescriptor*>& getDescriptorsByNameImpl();

		static void handleDuplicatedClass(ClassDescriptor *desc, void *address1, void *address2);
		static void onClassRegister(int classId);
		static void onClassUnregister(int classId);

		template<typename ClassType>
		static void registerClassImpl(const std::string& name, ClassDescriptor::Flags flags = ClassDescriptor::NONE, const std::string& category = "") {
			registerClassId<ClassType>(true);
			auto* desc = getDescriptorsImpl()[getClassId<ClassType>()];
			getDescriptorsByNameImpl().erase(desc->name);
			desc->name = name;
			getDescriptorsByNameImpl()[desc->name] = desc;
			desc->category = category;
			desc->flags = (ClassDescriptor::Flags)((int)desc->flags | (int)flags);
			if (!desc->wasRegisterCallbackInvoked) {
				desc->wasRegisterCallbackInvoked = true;
				onClassRegister(desc->classId);
			}
		}

		template<typename T>
		static int registerClassId(bool explicitRegistration) {
			auto& descriptors = getDescriptorsImpl();
			int(*registrationSourceAddress)(bool) { &Reflection::registerClassId<T> };

			size_t hashCode = typeid(T).hash_code();
			for (auto* desc : descriptors) {
				if (desc && desc->hashCode == hashCode) {
					if (explicitRegistration && desc->wasRegisteredExplicit) {
						if ((void*)registrationSourceAddress != desc->registrationSourceAddress) {
							handleDuplicatedClass(desc, desc->registrationSourceAddress, registrationSourceAddress);
						}
					}
					return desc->classId;
				}
			}

			int classId = (int)descriptors.size();
			for (int i = 0; i < descriptors.size(); i++) {
				if (!descriptors[i]) {
					classId = i;
					break;
				}
			}

			ClassDescriptor* desc;
			if constexpr (impl::is_vector<T>::value) {
				desc = new ClassDescriptorVector<T::value_type>();
				desc->elementType = descriptors[getClassId<T::value_type>()];
				desc->flags = ClassDescriptor::VECTOR;
			}
			else {
				desc = new ClassDescriptorT<T>();
			}

			desc->hashCode = hashCode;
			desc->size = sizeof(T);
			desc->classId = classId;
			desc->name = typeid(T).name();
			getDescriptorsByNameImpl()[desc->name] = desc;

			desc->registrationSourceAddress = registrationSourceAddress;
			desc->wasRegisteredExplicit = explicitRegistration;

			if (classId < descriptors.size()) {
				descriptors[classId] = desc;
			}
			else {
				descriptors.push_back(desc);
			}

			return desc->classId;
		}

	};

}

namespace tri::impl {

	class GlobalInitializationCallback {
	public:
		std::function<void()> onUninitialization;
		
		GlobalInitializationCallback(const std::function<void()> &onInitialization = nullptr, const std::function<void()>& onUninitialization = nullptr) {
			this->onUninitialization = onUninitialization;
			if (onInitialization) {
				onInitialization();
			}
		}

		~GlobalInitializationCallback() {
			if (onUninitialization) {
				onUninitialization();
			}
		}
	};

}

//helper macros
#define TRI_CONCAT_IMPL(a, b) a##b
#define TRI_CONCAT(a, b) TRI_CONCAT_IMPL(a, b)
#define TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line) base##_##line##_##counter
#define TRI_UNIQUE_IDENTIFIER_IMPL(base, counter, line) TRI_UNIQUE_IDENTIFIER_IMPL_2(base, counter, line)
#define TRI_UNIQUE_IDENTIFIER(base) TRI_UNIQUE_IDENTIFIER_IMPL(base, __COUNTER__, __LINE__)

#define TRI_CLASS_FLAGS(T, name, category, flags) static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ tri::Reflection::registerClass<T>(name, flags, category); }, [](){ tri::Reflection::unregisterClass<T>(); });
#define TRI_CLASS(T) TRI_CLASS_FLAGS(T, #T, "", tri::ClassDescriptor::Flags::NONE)

#define TRI_SYSTEM(T) TRI_CLASS_FLAGS(T, #T, "", tri::ClassDescriptor::Flags::SYSTEM)
#define TRI_SYSTEM_INSTANCE(T, ptr) TRI_SYSTEM(T) static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ tri::SystemManager::setSystemPointer<T>(&ptr); });
#define TRI_COMPONENT(T) TRI_CLASS_FLAGS(T, #T, "", tri::ClassDescriptor::Flags::COMPONENT)
#define TRI_COMPONENT_CATEGORY(T, category) TRI_CLASS_FLAGS(T, #T, category, tri::ClassDescriptor::Flags::COMPONENT)
#define TRI_ASSET(T) TRI_CLASS_FLAGS(T, #T, "", tri::ClassDescriptor::Flags::ASSET)
#define TRI_FUNCTION(T, func) static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ tri::Reflection::registerFunction<T>(#func, &T::func); });

#define TRI_PROPERTY_FLAGS(T, name, flags) static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ tri::Reflection::registerProperty<T, decltype(T::name)>(#name, offsetof(T, T::name), flags); });
#define TRI_PROPERTY_RANGE(T, name, min, max) static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ tri::Reflection::registerPropertyRange<T, decltype(T::name)>(#name, min, max); });
#define TRI_PROPERTY(T, name) TRI_PROPERTY_FLAGS(T, name, tri::PropertyDescriptor::NONE)
#define TRI_PROPERTIES1(T, property1) TRI_PROPERTY(T, property1)
#define TRI_PROPERTIES2(T, property1, property2) TRI_PROPERTY(T, property1) TRI_PROPERTIES1(T, property2)
#define TRI_PROPERTIES3(T, property1, property2, property3) TRI_PROPERTY(T, property1) TRI_PROPERTIES2(T, property2, property3)
#define TRI_PROPERTIES4(T, property1, property2, property3, property4) TRI_PROPERTY(T, property1) TRI_PROPERTIES3(T, property2, property3, property4)
#define TRI_PROPERTIES5(T, property1, property2, property3, property4, property5) TRI_PROPERTY(T, property1) TRI_PROPERTIES4(T, property2, property3, property4, property5)
#define TRI_PROPERTIES6(T, property1, property2, property3, property4, property5, property6) TRI_PROPERTY(T, property1) TRI_PROPERTIES5(T, property2, property3, property4, property5, property6)
#define TRI_PROPERTIES7(T, property1, property2, property3, property4, property5, property6, property7) TRI_PROPERTY(T, property1) TRI_PROPERTIES6(T, property2, property3, property4, property5, property6, property7)
#define TRI_PROPERTIES8(T, property1, property2, property3, property4, property5, property6, property7, property8) TRI_PROPERTY(T, property1) TRI_PROPERTIES7(T, property2, property3, property4, property5, property6, property7, property8)

#define TRI_ENUM1(T, value) static tri::impl::GlobalInitializationCallback TRI_UNIQUE_IDENTIFIER(init)([](){ tri::Reflection::registerEnumValue<T>(#value, T::value); });
#define TRI_ENUM2(T, value1, value2) TRI_ENUM1(T, value1) TRI_ENUM1(T, value2)
#define TRI_ENUM3(T, value1, value2, value3) TRI_ENUM1(T, value1) TRI_ENUM2(T, value2, value3)
#define TRI_ENUM4(T, value1, value2, value3, value4) TRI_ENUM1(T, value1) TRI_ENUM3(T, value2, value3, value4)
#define TRI_ENUM5(T, value1, value2, value3, value4, value5) TRI_ENUM1(T, value1) TRI_ENUM4(T, value2, value3, value4, value5)
#define TRI_ENUM6(T, value1, value2, value3, value4, value5, value6) TRI_ENUM1(T, value1) TRI_ENUM5(T, value2, value3, value4, value5, value6)
#define TRI_ENUM7(T, value1, value2, value3, value4, value5, value6, value7) TRI_ENUM1(T, value1) TRI_ENUM6(T, value2, value3, value4, value5, value6, value7)
#define TRI_ENUM8(T, value1, value2, value3, value4, value5, value6, value7, value8) TRI_ENUM1(T, value1) TRI_ENUM7(T, value2, value3, value4, value5, value6, value7, value8)
