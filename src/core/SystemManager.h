//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"
#include "Reflection.h"
#include "pch.h"

namespace tri {

	class SystemManager {
	public:
		template<typename T>
		static T* getSystem() {
			return (T*)getSystem(Reflection::getClassId<T>());
		}
		template<typename T>
		static T* addSystem() {
			return (T*)addSystem(Reflection::getClassId<T>());
		}
		template<typename T>
		static void removeSystem() {
			removeSystem(Reflection::getClassId<T>());
		}
		template<typename T>
		static void setSystemPointer(T** ptr) {
			setSystemPointer(Reflection::getClassId<T>(), (void**)ptr);
		}

		static System* getSystem(int classId);
		static System* addSystem(int classId);
		static void removeSystem(int classId, bool canAutoAddAgain = false);
		static void setSystemPointer(int classId, void** ptr);

		struct SystemHandle {
		public:
			System* system = nullptr;
			bool wasAutoAdd = false;
			bool wasInit = false;
			bool wasStartup = false;
			bool wasShutdown = false;
			bool pendingShutdown = false;
			bool active = true;
			void** instancePointer = nullptr;
			std::string name;
		};
		static SystemHandle* getSystemHandle(int classId);
		static bool hasPendingStartups();
		static bool hasPendingShutdowns();

		static void addNewSystems();
		static void removeAllSystems();
	};

}
