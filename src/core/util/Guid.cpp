//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Guid.h"

namespace tri {

	static uint8_t toHex(uint8_t v) {
		if (v < 16) {
			return "0123456789abcdef"[v];
		}
		else {
			return -1;
		}
	}

	static uint8_t fromHex(uint8_t v) {
		if (v >= '0' && v <= '9') {
			return v - '0';
		}
		else if (v >= 'a' && v <= 'f') {
			return v - 'a' + 10;
		}
		else {
			return -1;
		}
	}

    Guid::Guid() {
        Blob::operator=(0);
    }

    std::string Guid::toString() {
		int size = sizeof(*this);
		std::string result;
		result.resize(size * 2);
		for (int i = 0; i < size; i++) {
			uint8_t byte = *((uint8_t*)this + i);
			uint8_t v1 = (byte >> 4) & 0xf;
			uint8_t v2 = (byte >> 0) & 0xf;
			result[i * 2 + 0] = toHex(v1);
			result[i * 2 + 1] = toHex(v2);
		}
		return result;
    }

    bool Guid::fromString(const std::string& str) {
		int size = std::min(str.size() / 2, sizeof(*this));
		for (int i = 0; i < size; i++) {
			uint8_t v1 = fromHex(str[i * 2 + 0]);
			uint8_t v2 = fromHex(str[i * 2 + 1]);
			if (v1 == -1 || v2 == -1) {
				return false;
			}
			uint8_t byte = (v1 << 4) | (v2 << 0);
			*((uint8_t*)this + i) = byte;
		}

		for (int i = size; i < sizeof(*this); i++) {
			*((uint8_t*)this + i) = 0;
		}
		return true;
    }

}
