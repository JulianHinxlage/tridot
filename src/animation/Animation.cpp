//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Animation.h"
#include "core/core.h"
#include "engine/Color.h"
#include "engine/Serializer.h"
#include <glm/glm.hpp>

namespace tri {

	TRI_CLASS(KeyBlendMode);
	TRI_ENUM4(KeyBlendMode, NONE, STEP, LINEAR, SMOOTH);

	TRI_CLASS(PropertyFrame);
	TRI_PROPERTIES4(PropertyFrame, value, time, blend, relativeValue);

	TRI_CLASS(PropertySequence);
	TRI_PROPERTIES2(PropertySequence, entityId, frames);

	TRI_ASSET(Animation);
	TRI_PROPERTIES1(Animation, timeline);

	bool Animation::load(const std::string& file) {
		SerialData data;
		if (env->serializer->loadFromFile(data, file)) {
			timeline.clear();
			env->serializer->deserializeClass(this, data);
			return true;
		}
		return false;
	}

	bool Animation::save(const std::string& file) {
		SerialData data;
		env->serializer->saveToFile(data, file);
		env->serializer->serializeClass(this, data);
		return true;
	}

	float Animation::calculateMaxTime() {
		float max = 0;
		for (auto& seq : timeline) {
			for (auto& key : seq.frames) {
				if (key.time > max) {
					max = key.time;
				}
			}
		}
		return max;
	}

	void setPropertyDiscrete(int classId, void* result, void* v1, void* v2, float factor, KeyBlendMode blend) {
		auto* desc = Reflection::getDescriptor(classId);
		switch (blend) {
		case tri::NONE:
			if (factor <= 0.0f) {
				desc->copy(v1, result);
			}
			else {
				desc->copy(v2, result);
			}
			break;
		case tri::LINEAR:
		case tri::SMOOTH: {
		case tri::STEP:
			if (factor < 0.5f) {
				desc->copy(v1, result);
			}
			else {
				desc->copy(v2, result);
			}
			break;
		}
		default:
			break;
		}
	}

	template<typename T>
	void setPropertyNumeric(void* result, void* v1, void* v2, float factor, KeyBlendMode blend) {
		switch (blend) {
		case tri::NONE:
			if (factor <= 0.0f) {
				*(T*)result = *(T*)v1;
			}
			else {
				*(T*)result = *(T*)v2;
			}
			break;
		case tri::STEP:
			if (factor < 0.5f) {
				*(T*)result = *(T*)v1;
			}
			else {
				*(T*)result = *(T*)v2;
			}
			break;
		case tri::LINEAR:
			*(T*)result = *(T*)v1 + ((*(T*)v2 - *(T*)v1) * factor);
			break;
		case tri::SMOOTH: {
			float f = (std::sin((factor - 0.5) * 3.1415926) + 1.0f) / 2.0f;
			*(T*)result = *(T*)v1 + ((*(T*)v2 - *(T*)v1) * f);
			break;
		}
		default:
			break;
		}
	}

	void setProperty(int classId, void* result, void* v1, void* v2, float factor, KeyBlendMode blend) {
		if (classId == Reflection::getClassId<float>()) {
			setPropertyNumeric<float>(result, v1, v2, factor, blend);
		}
		else if (classId == Reflection::getClassId<int>()) {
			setPropertyNumeric<int>(result, v1, v2, factor, blend);
		}
		else if (classId == Reflection::getClassId<double>()) {
			setPropertyNumeric<double>(result, v1, v2, factor, blend);
		}
		else if (classId == Reflection::getClassId<glm::vec2>()) {
			setPropertyNumeric<glm::vec2>(result, v1, v2, factor, blend);
		}
		else if (classId == Reflection::getClassId<glm::vec3>()) {
			setPropertyNumeric<glm::vec3>(result, v1, v2, factor, blend);
		}
		else if (classId == Reflection::getClassId<glm::vec4>()) {
			setPropertyNumeric<glm::vec4>(result, v1, v2, factor, blend);
		}
		else if (classId == Reflection::getClassId<Color>()) {
			glm::vec4 cv1 = ((Color*)result)->vec();
			glm::vec4 cv2 = ((Color*)v1)->vec();
			glm::vec4 cv3 = ((Color*)v2)->vec();
			setPropertyNumeric<glm::vec4>(&cv1, &cv2, &cv3, factor, blend);
			(*(Color*)result) = cv1;
		}
		else {
			setPropertyDiscrete(classId, result, v1, v2, factor, blend);
		}
	}

	void Animation::apply(float time, EntityId id) {
		for (auto& seq : timeline) {
			if (seq.frames.size() > 0) {
				PropertyFrame* prev = &seq.frames[0];
				for (int i = 0; i < seq.frames.size(); i++) {
					PropertyFrame* key = &seq.frames[i];
					if (i > 0) {
						float factor = time - prev->time;
						if (factor >= 0 && factor <= key->time - prev->time) {
							factor /= (key->time - prev->time);

							EntityId propId = seq.entityId == -1 ? id : seq.entityId;

							void* prop = prev->value.getProperty(propId, env->world);
							if (prop && key->value.value.classId >= 0) {
								setProperty(key->value.value.classId, prop, prev->value.value.get(), key->value.value.get(), factor, key->blend);
							}
						}
					}

					prev = key;
				}
			}
		}
	}

}
