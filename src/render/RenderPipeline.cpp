//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderPipeline.h"
#include "core/core.h"

namespace tri {

	RenderPipeline::RenderPipeline(Ref<Mesh> quad) {
		this->quad = quad;
	}

	void RenderPipeline::addRenderPass(Ref<RenderPass> pass, const std::string& name, Ref<RenderPass> previous) {
		if (previous) {
			for (int i = 0; i < passes.size(); i++) {
				if (passes[i].pass == previous) {
					passes.insert(passes.begin() + i, { pass, name });
					return;
				}
			}
		}
		passes.push_back({pass, name});
	}

	void RenderPipeline::removeRenderPass(const std::string& name) {
		for (int i = 0; i < passes.size(); i++) {
			if (passes[i].name == name) {
				passes.erase(passes.begin() + i);
				i--;
			}
		}
	}

	void RenderPipeline::removeRenderPass(Ref<RenderPass> pass) {
		for (int i = 0; i < passes.size(); i++) {
			if (passes[i].pass == pass) {
				passes.erase(passes.begin() + i);
				i--;
			}
		}
	}

	Ref<RenderPass> RenderPipeline::getRenderPass(const std::string& name) {
		for (auto& pass : passes) {
			if (pass.name == name) {
				return pass.pass;
			}
		}
		return nullptr;
	}

	Ref<FrameBuffer> RenderPipeline::executePipeline(Ref<FrameBuffer> target) {
		Ref<FrameBuffer> output = input;
		for (int i = 0; i < passes.size(); i++) {
			auto& pass = passes[i];
			if(pass.pass){
				if (i == passes.size() - 1 && target != nullptr) {
					target->bind();
					output = target;
				}
				else {
					pass.pass->output->bind();
					output = pass.pass->output;
				}
				pass.pass->shader->bind();
				int slot = 0;
				for (auto& input : pass.pass->inputs) {
					if (input) {
						input->bind(slot);
					}
					slot++;
				}
				quad->vertexArray.submit();
			}
		}
		return output;
	}

}
