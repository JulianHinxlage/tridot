//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "DrawList.h"
#include "RenderBatch.h"
#include "ShaderStructs.h"
#include "render/objects/Mesh.h"
#include "render/objects/Material.h"
#include "render/objects/FrameBuffer.h"
#include <glm/glm.hpp>

namespace tri {

	class Renderer : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;
		void submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color = color::white, EntityId id = -1);

	private:
		Ref<Mesh> defaultMesh;
		Ref<Texture> defaultTexture;
		Ref<Material> defaultMaterial;
		Ref<Shader> defaultShader;
		glm::mat4 projection;
		Ref<FrameBuffer> frameBuffer;

		DrawList drawList;
		RenderBatchList batches;
		EnvironmentData envData;
		Ref<Buffer> envBuffer;

		void setupFrameBuffer(Ref<FrameBuffer>& frameBuffer);
	};

}
