//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Animation.h"
#include "core/core.h"
#include "engine/Color.h"
#include "engine/Serializer.h"
#include <glm/glm.hpp>

namespace tri {

	TRI_CLASS(KeyFrameBlendMode);
	TRI_ENUM4(KeyFrameBlendMode, NONE, STEP, LINEAR, SMOOTH);

	TRI_CLASS(KeyFrameProperty);
	TRI_PROPERTIES1(KeyFrameProperty, value);

	TRI_CLASS(KeyFrame);
	TRI_PROPERTIES3(KeyFrame, time, blend, properties);

	TRI_ASSET(Animation);
	TRI_PROPERTIES1(Animation, keyFrames);

	bool Animation::load(const std::string& file) {
		SerialData data;
		if (env->serializer->loadFromFile(data, file)) {
			keyFrames.clear();
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
		for (auto& key : keyFrames) {
			if (key.time > max) {
				max = key.time;
			}
		}
		return max;
	}

	void setPropertyDiscrete(int classId, void* result, void* v1, void* v2, float factor, KeyFrameBlendMode blend) {
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
	void setPropertyNumeric(void* result, void* v1, void* v2, float factor, KeyFrameBlendMode blend) {
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

	void setProperty(int classId, void* result, void* v1, void* v2, float factor, KeyFrameBlendMode blend) {
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
		for (int i = 1; i < keyFrames.size(); i++) {
			auto& frame = keyFrames[i];
			auto& prevFrame = keyFrames[i-1];
			if (time <= frame.time && time >= prevFrame.time) {

				for (auto& prop : frame.properties) {


					void* value1 = nullptr;
					float factor = 0;
					for (int j = i - 1; j >= 0; j--) {
						auto& prev = keyFrames[j];

						for (auto& p : prev.properties) {
							if (p.value.classId == prop.value.classId) {
								if (p.value.propertyIndex == prop.value.propertyIndex) {
									value1 = p.value.value.get();
									factor = time - prev.time / (frame.time - prev.time);
									break;
								}
							}
						}
						if (value1 != nullptr) {
							break;
						}
					}
					if (value1) {
						void* value2 = prop.value.value.get();
						void* result = prop.value.getProperty(id, env->world);
						int propClassId = prop.value.value.classId;
						if (value2 && result) {
							setProperty(propClassId, result, value1, value2, factor, frame.blend);
						}
					}
				}
			}
		}
	}

}
