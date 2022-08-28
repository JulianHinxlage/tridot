//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "IndexMap.h"
#include "RenderPipeline.h"
#include "render/objects/Mesh.h"
#include "render/objects/Material.h"
#include "render/objects/FrameBuffer.h"
#include "render/objects/BatchBuffer.h"

namespace tri {

	class RenderBatch {
	public:
		Shader *shader;
		Mesh* mesh;

		IndexMap<Texture> textures;
		IndexMap<Material> materials;

		Ref<BatchBuffer> instanceBuffer;
		Ref<BatchBuffer> materialBuffer;
		Ref<VertexArray> vertexArray;
		int meshChangeCounter;

		Ref<ShaderState> shaderState;
		Ref<Buffer> environmentBuffer;
		Texture* defaultTexture;

		RenderBatch();
		void init(Shader *shader, Mesh *mesh);
		bool isInitialized();
		void add(const glm::mat4 &transform, Material *material, Color color, EntityId id);
		void submit(FrameBuffer *frameBuffer, RenderPipeline::RenderPass pass);
		void reset();
	private:
		void updateMesh();
	};

	class RenderBatchList {
	public:
		std::unordered_map<Shader*, std::unordered_map<Mesh*, Ref<RenderBatch>>> batches;
		
		RenderBatch* get(Shader* shader, Mesh* mesh);
		void clear();
	};

}
