//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "DynamicObjectBuffer.h"
#include "core/core.h"

namespace tri {

	DynamicObjectBuffer::DynamicObjectBuffer() {
		data = nullptr;
		classId = -1;
		count = 0;
	}

	DynamicObjectBuffer::DynamicObjectBuffer(const DynamicObjectBuffer& buffer) {
		operator=(buffer);
	}

	DynamicObjectBuffer::DynamicObjectBuffer(DynamicObjectBuffer&& buffer) {
		data = buffer.data;
		classId = buffer.classId;
		count = buffer.count;
		buffer.data = nullptr;
		buffer.count = 0;
		buffer.classId = -1;
	}

	DynamicObjectBuffer::~DynamicObjectBuffer() {
		clear();
	}

	void DynamicObjectBuffer::operator=(const DynamicObjectBuffer& buffer) {
		data = nullptr;
		classId = -1;
		count = 0;
		set(buffer.classId, buffer.data, buffer.count);
	}

	void DynamicObjectBuffer::set(int classId, const void* ptr, int count) {
		clear();
		if (classId != -1) {
			this->classId = classId;
			this->count = count;
			auto* desc = Reflection::getDescriptor(classId);
			data = new uint8_t[count * desc->size];
			if (ptr) {
				desc->copy(ptr, data, count);
			}
			else {
				desc->construct(data, count);
			}
		}
	}

	void DynamicObjectBuffer::get(void* ptr) const {
		auto* desc = Reflection::getDescriptor(classId);
		desc->copy(data, ptr);
	}

	void* DynamicObjectBuffer::get() const {
		return data;
	}

	void DynamicObjectBuffer::clear() {
		if (data) {
			auto* desc = Reflection::getDescriptor(classId);
			if (desc) {
				desc->destruct(data, count);
			}
			else {
				env->console->warning("can't properly free memory for component");
			}
			delete data;
		}
		data = nullptr;
		count = 0;
		classId = -1;
	}

}
