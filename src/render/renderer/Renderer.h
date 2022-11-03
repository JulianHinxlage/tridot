//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include "DrawList.h"
#include "RenderBatch.h"
#include "ShaderStructs.h"
#include "ViewFrustum.h"
#include "engine/Camera.h"
#include "engine/Light.h"
#include "engine/Transform.h"
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
		Ref<FrameBuffer>& getGBuffer();
		Ref<FrameBuffer> &getBloomBuffer();

	private:
		Ref<Mesh> quadMesh;
		Ref<Texture> defaultTexture;
		Ref<Material> defaultMaterial;

		DrawList drawList;
		DrawList transparencyDrawList;
		RenderBatchList batches;
		RenderBatchList transparencyBatches;
		RenderBatchList shadowBatches;
		EnvironmentData envData;
		Ref<Buffer> envBuffer;
		glm::vec3 eyePosition;

		EnvironmentData shadowEnvData;
		Ref<Buffer> shadowEnvBuffer;

		Ref<Shader> geometryShader;
		Ref<Shader> ambientLightShader;
		Ref<Shader> directionalLightShader;
		Ref<Shader> pointLightShader;
		Ref<Shader> spotLightShader;
		Ref<Shader> bloomShader;
		Ref<Shader> blurShader;
		Ref<Shader> compositShader;
		Ref<Shader> skyboxShader;
		Ref<Shader> ssaoShader;
		Ref<Shader> shadowShader;

		Ref<Mesh> sphereMesh;
		Ref<Mesh> coneMesh;
		Ref<Mesh> cubeMesh;

		Ref<FrameBuffer> gBuffer;
		Ref<FrameBuffer> lightAccumulationBuffer;
		Ref<FrameBuffer> transparencyBuffer;
		Ref<FrameBuffer> bloomBuffer1;
		Ref<FrameBuffer> bloomBuffer2;
		Ref<FrameBuffer> ssaoBuffer;

		ViewFrustum frustum;

		std::vector<glm::vec3> ssaoSamples;
		Ref<Texture> ssaoNoise;

		std::vector<FrameBufferAttachmentSpec> gBufferSpec;
		std::vector<FrameBufferAttachmentSpec> lightAccumulationSpec;
		std::vector<FrameBufferAttachmentSpec> bloomBufferSpec;
		std::vector<FrameBufferAttachmentSpec> ssaoBufferSpec;
		std::vector<FrameBufferAttachmentSpec> shadowMapSpec;

		class LightBatch {
		public:
			class Instance {
			public:
				glm::mat4 transform;
				glm::vec3 position;
				glm::vec3 direction;
				Color color;
				float intensity;
				float range;
				float falloff;
				float spotAngle;
			};

			Ref<BatchBuffer> instanceBuffer;
			Ref<VertexArray> vertexArray;
			bool hasPrepared = false;
		};
		LightBatch pointLightBatch;
		LightBatch spotLightBatch;


		void setupSpecs();
		bool updateFrameBuffer(Ref<FrameBuffer>& frameBuffer, const std::vector<FrameBufferAttachmentSpec> &spec, glm::vec2 size);
		void prepareTransparencyBuffer();
		bool prepareLightBatches();
		void submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color = color::white, EntityId id = -1);
		void submitMeshes();
		void submitBatches(Camera& c);
		void submitLights(const Camera &camera);
		bool submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const AmbientLight& light, const Transform& transform, const Camera& camera);
		bool submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const DirectionalLight& light, const Transform& transform, const Camera& camera);
		bool submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const PointLight& light, const Transform& transform, const Camera& camera);
		bool submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const SpotLight& light, const Transform& transform, const Camera& camera);

		void submitPointLightBatch();
		void submitSpotLightBatch();
		void submitBloom();
		void submitDisplay();
		void submitSkyBox(const Camera& camera);
		void submitSSAO();
		void submitShadows();
	};

}
