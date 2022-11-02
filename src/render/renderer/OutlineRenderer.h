//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "DrawList.h"
#include "RenderBatch.h"
#include <glm/glm.hpp>

namespace tri {

	class OutlineRenderer : public System {
	public:
		Color outlineColor;

		void startup() override;
		void submit(const glm::mat4 &transform, Mesh *mesh);
		void submitBatches(const glm::mat4& viewProjection);
		Ref<FrameBuffer>& getFrameBuffer();

	private:
		Ref<Material> material;
		Ref<FrameBuffer> meshesFrameBuffer;
		Ref<FrameBuffer> outlineFrameBuffer;
		Ref<Shader> outlineShader;
		Ref<Shader> meshShader;
		Ref<Mesh> quad;
		DrawList drawList;
		RenderBatchList batches;
	};

}