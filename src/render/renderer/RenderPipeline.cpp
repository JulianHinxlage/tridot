//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RenderPipeline.h"
#include "engine/MeshComponent.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "engine/RuntimeMode.h"
#include "window/RenderContext.h"
#include "window/Viewport.h"
#include <GL/glew.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

	TRI_SYSTEM_INSTANCE(RenderPipeline, env->renderPipeline);

	void RenderPipeline::init() {
		auto* job = env->jobManager->addJob("Render");
		job->addSystem<RenderPipeline>();
		env->runtimeMode->setActiveSystem(RuntimeMode::EDIT, "RenderPipeline", true);
		env->runtimeMode->setActiveSystem(RuntimeMode::PAUSED, "RenderPipeline", true);

		addStep(Ref<Step>::make(), RenderPass::PREPARE, true, false);
		addStep(Ref<Step>::make(), RenderPass::OPAQUE, true, false);
		addStep(Ref<Step>::make(), RenderPass::TRANSPARENCY, true, false);
		addStep(Ref<Step>::make(), RenderPass::SHADOWS, true, false);
		addStep(Ref<Step>::make(), RenderPass::LIGHTING, true, false);
		addStep(Ref<Step>::make(), RenderPass::POST_PROCESSING, true, false);

		processedPrepared = true;
	}

	void RenderPipeline::startup() {
		//glew needs to be initialized in every dll, so for now we do it here
		//we cant use the RenderContext class, because the functions are implemented in another dll
		glewInit();
	}

	void RenderPipeline::tick() {
		env->eventManager->postTick.addListener([&]() {
			if (processedPrepared) {
				TRI_PROFILE("prepare render pipeline");
				std::unique_lock<std::mutex> lock(mutex);
				preparedSteps.swap(steps);

				/*
				//the destruction of the steps may take a while on the post tick hot path
				//thus we move the destruction to a task thread
				static std::vector<Ref<Step>> freeSteps;
				freeSteps.swap(steps);
				env->threadManager->addTask([&]() {
					freeSteps.clear();
				});
				*/ 

				steps.clear();
				for (auto& step : preparedSteps) {
					if (step->fixed) {
						steps.push_back(step->copy(true, true));
					}
				}

				processedPrepared = false;
			}

		}, true);

		statistics = RenderSettings::Statistics();
		for (auto& step : preparedSteps) {
			step->execute(*this);
		}
		processedPrepared = true;
		env->renderSettings->statistics.triangleCount = statistics.triangleCount;
		env->renderSettings->statistics.instanceCount = statistics.instanceCount;
		env->renderSettings->statistics.drawCallCount = statistics.drawCallCount;
		
		refFreeList.clear();
		callbackFreeList.clear();
		FrameBuffer::unbind();
	}

	void RenderPipeline::shutdown() {
		steps.clear();
		preparedSteps.clear();
		refFreeList.clear();
		callbackFreeList.clear();
	}

	void RenderPipeline::addStep(Ref<Step> step, RenderPass pass, bool fixed, bool sort) {
		std::unique_lock<std::mutex> lock(mutex);
		step->fixed = fixed;
		step->pass = pass;

		if (sort) {
			int index = -1;
			for (int i = 0; i < steps.size(); i++) {
				if (steps[i]->pass == pass) {
					index = i;
				}
			}
			if (index == -1) {
				index = steps.size() - 1;
			}
			if (index >= 0 && index < steps.size()) {
				steps[index]->steps.push_back(step);
			}
			else {
				steps.push_back(step);
			}
		}
		else {
			steps.push_back(step);
		}
	}

	void RenderPipeline::addCommandStep(Command command, RenderPass pass, bool fixed) {
		Ref<RenderPipeline::StepCommand> step(true);
		step->command = command;
		env->renderPipeline->addStep((Ref<RenderPipeline::Step>)step, pass, fixed);
	}

	void RenderPipeline::addCallbackStep(const std::function<void()>& callback, RenderPass pass, bool fixed) {
		Ref<RenderPipeline::StepCallback> step(true);
		step->callback = callback;
		env->renderPipeline->addStep((Ref<RenderPipeline::Step>)step, pass, fixed);
	}

	Ref<RenderPipeline::StepDrawCall> RenderPipeline::addDrawCallStep(RenderPass pass, bool fixed) {
		Ref<RenderPipeline::StepDrawCall> step(true);
		env->renderPipeline->addStep((Ref<RenderPipeline::Step>)step, pass, fixed);
		return step;
	}

	RenderPipeline::Step::Step() {
		type = NONE;
		pass = UNDEFINED;
		fixed = false;
		name = "";
	}

	Ref<RenderPipeline::Step> RenderPipeline::Step::copy(bool copySteps, bool copyOnlyFixed) {
		Ref<Step> result = Ref<Step>::make();
		copy(result.get(), copySteps, copyOnlyFixed);
		return result;
	}

	void RenderPipeline::Step::copy(Step *result, bool copySteps, bool copyOnlyFixed) {
		result->type = type;
		result->pass = pass;
		result->fixed = fixed;
		result->name = name;
		if (copySteps) {
			for (auto& i : steps) {
				if (i->fixed || !copyOnlyFixed) {
					result->steps.push_back(i->copy(copySteps, copyOnlyFixed));
				}
			}
		}
	}

	void RenderPipeline::Step::execute(RenderPipeline& pipeline) {
		for (auto& step : steps) {
			step->execute(pipeline);
		}
	}

	RenderPipeline::StepDrawCall::StepDrawCall() {
		type = DRAW_CALL;
		instanceCount = -1;
	}

	void RenderPipeline::StepDrawCall::execute(RenderPipeline& pipeline) {
		if (!shader) {
			return;
		}
		if (!vertexArray) {
			return;
		}

		if (frameBuffer) {
			frameBuffer->bind();
		}
		else {
			FrameBuffer::unbind();
		}

		shader->bind();
		if (shaderState) {
			shaderState->apply(shader);
		}

		for (int i = 0; i < textures.size(); i++) {
			if (textures[i]) {
				textures[i]->bind(i);
			}
		}

		for (int i = 0; i < buffers.size(); i++) {
			if (buffers[i]) {
				buffers[i]->update();
			}
		}

		{
			TracyGpuZone("submit");
			vertexArray->submit(-1, instanceCount);
		}

		//statistics
		pipeline.statistics.drawCallCount++;
		if (instanceCount == -1) {
			pipeline.statistics.instanceCount++;
			pipeline.statistics.triangleCount += vertexArray->getVertexCount() / 3;
		}
		else {
			pipeline.statistics.instanceCount += instanceCount;
			pipeline.statistics.triangleCount += vertexArray->getVertexCount() / 3 * instanceCount;
		}

		Step::execute(pipeline);
	}

	Ref<RenderPipeline::Step> RenderPipeline::StepDrawCall::copy(bool copySteps, bool copyOnlyFixed) {
		Ref<StepDrawCall> result = Ref<StepDrawCall>::make();

		result->frameBuffer = frameBuffer;
		result->shader = shader;
		result->shaderState = Ref<ShaderState>::make(*shaderState);
		result->vertexArray = vertexArray;
		result->textures = textures;
		result->buffers = buffers;
		result->instanceCount = instanceCount;

		Step::copy(result.get(), copySteps, copyOnlyFixed);
		return (Ref<Step>)result;
	}

	RenderPipeline::StepCommand::StepCommand() {
		type = COMMAND;
	}

	Ref<RenderPipeline::Step> RenderPipeline::StepCommand::copy(bool copySteps, bool copyOnlyFixed) {
		Ref<StepCommand> result = Ref<StepCommand>::make();
		result->command = command;
		Step::copy(result.get(), copySteps, copyOnlyFixed);
		return (Ref<Step>)result;
	}

	void RenderPipeline::StepCommand::execute(RenderPipeline& pipeline) {
		switch (command) {
		case Command::DEPTH_ON:
			RenderContext::setDepth(true);
			break;
		case Command::DEPTH_OFF:
			RenderContext::setDepth(false);
			break; 
		case Command::BLEND_ON:
			RenderContext::setBlend(true);
			break; 
		case Command::BLEND_OFF:
			RenderContext::setBlend(false);
			break; 
		case Command::CULL_FRONT:
			RenderContext::setCull(true, true);
			break; 
		case Command::CULL_BACK:
			RenderContext::setCull(true, false);
			break;
		case Command::CULL_OFF:
			RenderContext::setCull(false);
			break;
		default:
			break;
		}
		Step::execute(pipeline);
	}

	RenderPipeline::StepCallback::StepCallback() {
		type = CALLBACK;
	}

	RenderPipeline::StepCallback::~StepCallback() {
		env->renderPipeline->freeOnThread(callback);
	}

	void RenderPipeline::StepCallback::execute(RenderPipeline& pipeline) {
		if (callback) {
			callback();
		}
		Step::execute(pipeline);
	}

	Ref<RenderPipeline::Step> RenderPipeline::StepCallback::copy(bool copySteps, bool copyOnlyFixed) {
		Ref<StepCallback> result = Ref<StepCallback>::make();
		result->callback = callback;
		Step::copy(result.get(), copySteps, copyOnlyFixed);
		return (Ref<Step>)result;
	}

}
