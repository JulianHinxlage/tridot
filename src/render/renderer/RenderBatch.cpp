//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RenderBatch.h"
#include "ShaderStructs.h"

namespace tri {

	RenderBatch::RenderBatch() {
		shader = nullptr;
		mesh = mesh;
		instanceBuffer = nullptr;
		materialBuffer = nullptr;
		vertexArray = nullptr;
		meshChangeCounter = 0;
	}

	void RenderBatch::init(Shader* shader, Mesh* mesh) {
		this->shader = shader;
		this->mesh = mesh;
		instanceBuffer = Ref<BatchBuffer>::make();
		materialBuffer = Ref<BatchBuffer>::make();
		instanceBuffer->init(sizeof(InstanceData));
		materialBuffer->init(sizeof(MaterialData), BufferType::UNIFORM_BUFFER);
		shaderState = Ref<ShaderState>::make();

		updateMesh();
	}

	void RenderBatch::updateMesh() {
		meshChangeCounter = mesh->changeCounter;
		vertexArray = Ref<VertexArray>::make(mesh->vertexArray);
		vertexArray->addVertexBuffer(instanceBuffer->buffer, {
			//transform
			{FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4},
			//color
			{UINT8, 4, true},
			//material index
			{FLOAT, 1},
			//id
			{UINT8, 4, true},
		}, 1);
	}

	bool RenderBatch::isInitialized() {
		return vertexArray.get() != nullptr;
	}

	void RenderBatch::add(const glm::mat4& transform, Material* material, Color color, EntityId id) {
		int mIndex = materials.getIndex(material);

		if (mIndex >= materialBuffer->size()) {
			MaterialData *mData = (MaterialData*)materialBuffer->next();

			mData->texture = textures.getIndex(material->texture.get());
			mData->normalMap = textures.getIndex(material->normalMap.get());
			mData->roughnessMap = textures.getIndex(material->roughnessMap.get());
			mData->metallicMap = textures.getIndex(material->metallicMap.get());
			mData->ambientOcclusionMap = textures.getIndex(material->ambientOcclusionMap.get());

			mData->color = material->color.vec();
			mData->mapping = (int)material->mapping;
			mData->roughness = material->roughness;
			mData->metallic = material->metallic;
			mData->normalMapFactor = material->normalMapFactor;
			mData->emissive = material->emissive;

			mData->textureOffset = material->textureOffset + material->offset;
			mData->textureScale = material->textureScale * material->scale;
			mData->normalMapOffset = material->normalMapOffset + material->offset;
			mData->normalMapScale = material->normalMapScale * material->scale;
			mData->roughnessMapOffset = material->roughnessMapOffset + material->offset;
			mData->roughnessMapScale = material->roughnessMapScale * material->scale;
			mData->metallicMapOffset = material->metallicMapOffset + material->offset;
			mData->metallicMapScale = material->metallicMapScale * material->scale;
			mData->ambientOcclusionMapOffset = material->ambientOcclusionMapOffset + material->offset;
			mData->ambientOcclusionMapScale = material->ambientOcclusionMapScale * material->scale;
		}

		InstanceData *iData = (InstanceData*)instanceBuffer->next();
		iData->transform = transform;
		iData->color = color;
		iData->materialIndex = (float)mIndex;
		iData->id = id;
	}

	void RenderBatch::submit(FrameBuffer* frameBuffer, RenderPipeline::RenderPass pass) {
		//update the vertex array if the mesh has changes (e.g. asset reload)
		if (meshChangeCounter != mesh->changeCounter) {
			env->renderPipeline->addCallbackStep([&]() {
				updateMesh();
			});
			instanceBuffer->swapBuffers();
			materialBuffer->swapBuffers();
			return;
		}

		auto dc = env->renderPipeline->addDrawCallStep(pass);
		dc->frameBuffer = frameBuffer;
		dc->shader = shader;
		dc->vertexArray = vertexArray.get();
		dc->buffers.push_back(materialBuffer.get());
		dc->buffers.push_back(instanceBuffer.get());

		for (auto& tex : textures.elements) {
			if (tex != nullptr) {
				dc->textures.push_back(tex);
			}
			else {
				dc->textures.push_back(defaultTexture);
			}
		}

		dc->instanceCount = instanceBuffer->size();
		dc->shaderState = Ref<ShaderState>::make();

		dc->shaderState = shaderState;
		std::vector<int> textureSlots;
		for (int i = 0; i < textures.elements.size(); i++) {
			textureSlots.push_back(i);
		}
		dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
		dc->shaderState->set("uMaterials", materialBuffer->buffer.get());
		if (environmentBuffer) {
			dc->shaderState->set("uEnvironment", environmentBuffer.get());
		}

		instanceBuffer->swapBuffers();
		materialBuffer->swapBuffers();
	}

	void RenderBatch::reset() {
		textures.reset();
		materials.reset();
	}

	RenderBatch* RenderBatchList::get(Shader* shader, Mesh* mesh) {
		auto& batch = batches[shader][mesh];
		if (!batch) {
			batch = Ref<RenderBatch>::make();
			env->renderPipeline->addCallbackStep([batch, shader, mesh]() {
				batch->init(shader, mesh);
			});
		}
		return batch.get();
	}

	void RenderBatchList::clear() {
		batches.clear();
	}

}
