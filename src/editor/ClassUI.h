//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/System.h"
#include "core/Reflection.h"

namespace tri {

	class ClassUI : public System {
	public:
		bool enableReferenceEdit;

		void init() override;
		bool draw(int classId, void* ptr, const char* label = nullptr, void* min = nullptr, void* max = nullptr, bool multiValue = false, bool drawHidden = false);
		void addClassUI(int classId, const std::function<bool(const char* label, void* value, void* min, void* max, bool multiValue)>& callback);

		template<typename T>
		void draw(T &t, const char *label = nullptr) {
			draw(Reflection::getClassId<T>(), &t, label);
		}

		template<typename T>
		void addClassUI(const std::function<bool(const char* label, T* value, T* min, T* max, bool multiValue)>& callback) {
			addClassUI(Reflection::getClassId<T>(), [callback](const char* label, void* value, void* min, void* max, bool multiValue) {
				return callback(label, (T*)value, (T*)min, (T*)max, multiValue);
			});
		}

		bool dragTarget(int classId, void* ptr);
		bool dragSource(int classId, const void* ptr);
		bool dragTarget(int classId, std::string &str);
		bool dragSource(int classId, const std::string &str);

	private:
		std::vector<std::function<bool(const char *label, void* value, void* min, void* max, bool multiValue)>> callbacks;
	};

}