//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

	class DynamicObjectBuffer {
	public:
		int classId;
		uint8_t* data;
		int count;

		DynamicObjectBuffer();
		DynamicObjectBuffer(const DynamicObjectBuffer& buffer);
		~DynamicObjectBuffer();
		void set(int classId, const void* ptr = nullptr, int count = 1);
		void get(void* ptr) const;
		void* get() const;
		void clear();
	};

}
