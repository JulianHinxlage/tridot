//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/util/Ref.h"
#include "FrameBuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"

namespace tri {

	class RenderPass {
	public:
		Ref<Shader> shader;
		Ref<FrameBuffer> output;
		std::vector<Ref<Texture>> inputs;
	};

	class RenderPipeline {
	public:
		Ref<FrameBuffer> input;

		RenderPipeline(Ref<Mesh> quad);
		void addRenderPass(Ref<RenderPass> pass, const std::string& name, Ref<RenderPass> previous = nullptr);
		void removeRenderPass(const std::string& name);
		void removeRenderPass(Ref<RenderPass> pass);
		Ref<RenderPass> getRenderPass(const std::string& name);
		Ref<FrameBuffer> executePipeline(Ref<FrameBuffer> target = nullptr);

	private:
		class Pass {
		public:
			Ref<RenderPass> pass;
			std::string name;
		};

		std::vector<Pass> passes;
		Ref<Mesh> quad;
	};

}
