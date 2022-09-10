//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "render/objects/Mesh.h"
#include <glm/glm.hpp>

namespace tri {

	class ViewFrustum {
	public:
		glm::mat4 viewProjectionMatrix;
		bool inClipSpace(const glm::vec4& p);

		bool inFrustum(const glm::mat4& transform, const Mesh* mesh) {
			return inFrustumSphere(transform, mesh);
		}

		bool inFrustumBox(const glm::mat4& transform, const Mesh* mesh);
		bool inFrustumSphere(const glm::mat4 &transform, const Mesh *mesh);
	};

}
