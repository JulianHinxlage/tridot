//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

#include "render/objects/Mesh.h"

namespace tri {

	class MeshFactory : public System {
	public:
		Ref<Mesh> generateQuad(float width = 1, float height = 1, glm::vec3 normal = { 0, 1, 0 });
		Ref<Mesh> generateRegularPoligon(int vertexCount = 3, glm::vec3 normal = { 0, 1, 0 });
		Ref<Mesh> generateCube(float width = 1, float height = 1, float depth = 1);
		Ref<Mesh> generateCubeSphere(int faceSections = 3);
	};

}
