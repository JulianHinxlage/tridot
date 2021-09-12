//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/config.h"
#include "render/FrameBuffer.h"
#include "render/RenderPipeline.h"
#include <glm/glm.hpp>

namespace tri {

	class Camera {
	public:
		glm::vec3 forward;
		glm::vec3 up;
		glm::vec3 right;
		glm::mat4 projection;

		//direct geometry output
		Ref<FrameBuffer> target;

		//output after other passes
		Ref<FrameBuffer> output;

		Ref<RenderPipeline> pipeline;
		bool isPrimary;
		bool active;

		enum Type {
			PERSPECTIVE,
			ORTHOGRAPHIC,
		};
		Type type;
		float near;
		float far;
		float fieldOfView;
		float aspectRatio;

		Camera(Type type = PERSPECTIVE, bool isPrimary = false);
	};

}
