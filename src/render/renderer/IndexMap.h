//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

	template<typename T>
	class IndexMap{
	public:
		std::vector<T*> elements;
		std::unordered_map<T*, int> indexMap;

		int getIndex(T *element) {
			auto entry = indexMap.find(element);
			if (entry != indexMap.end()) {
				return entry->second;
			}

			int index = elements.size();
			elements.push_back(element);
			indexMap[element] = index;
			return index;
		}

		T* getElement(int index) {
			if (index >= 0 && index < elements.size()) {
				return elements[index];
			}
			else {
				return nullptr;
			}
		}

		void reset() {
			elements.clear();
			indexMap.clear();
		}
	};

}
