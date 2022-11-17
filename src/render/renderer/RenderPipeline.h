//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "RenderSettings.h"
#include "render/objects/FrameBuffer.h"
#include "render/objects/Shader.h"
#include "render/objects/ShaderState.h"
#include "render/objects/VertexArray.h"
#include "render/objects/BatchBuffer.h"

namespace tri {

	class RenderPipeline : public System {
	public:
		enum RenderPass {
			UNDEFINED,
			PREPARE,
			GEOMETRY,
			SHADOWS,
			LIGHTING,
			TRANSPARENCY,
			POST_PROCESSING,
			DISPLAY,
			POST_RENDER,
		};

		enum StepType {
			NONE,
			COMMAND,
			DRAW_CALL,
			CALLBACK,
		};

		enum Command {
			NOOP,
			DEPTH_ON,
			DEPTH_OFF,
			BLEND_ON,
			BLEND_OFF,
			BLEND_ADDITIVE,
			CULL_OFF,
			CULL_BACK,
			CULL_FRONT,
		};

		class Step {
		public:
			RenderPass pass;
			StepType type;
			bool fixed;
			std::string name;
			std::vector<Ref<Step>> steps;

			Step();
			virtual Ref<Step> copy(bool copySteps = true, bool copyOnlyFixed = false);
			virtual void execute(RenderPipeline& pipeline);
		protected:
			void copy(Step *result, bool copySteps = true, bool copyOnlyFixed = false);
		};

		class StepCommand : public Step {
		public:
			Command command;

			StepCommand();
			virtual Ref<Step> copy(bool copySteps = true, bool copyOnlyFixed = false);
			virtual void execute(RenderPipeline& pipeline);
		};

		class StepDrawCall : public Step {
		public:
			FrameBuffer *frameBuffer;
			Shader *shader;
			Ref<ShaderState> shaderState;
			VertexArray *vertexArray;
			std::vector<Texture*> textures;
			std::vector<Texture*> images;
			std::vector<BatchBuffer*> buffers;
			int instanceCount;

			StepDrawCall();
			virtual void execute(RenderPipeline& pipeline);
			virtual Ref<Step> copy(bool copySteps = true, bool copyOnlyFixed = false);
		};

		class StepCallback : public Step {
		public:
			std::function<void()> callback;

			StepCallback();
			~StepCallback();
			virtual void execute(RenderPipeline& pipeline);
			virtual Ref<Step> copy(bool copySteps = true, bool copyOnlyFixed = false);
		};

		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		void addStep(Ref<Step> step, RenderPass pass = PREPARE, bool fixed = false, bool sort = true);
		void addCommandStep(Command command, RenderPass pass = PREPARE, bool fixed = false);
		void addCallbackStep(const std::function<void()> &callback, RenderPass pass = PREPARE, bool fixed = false);
		Ref<StepDrawCall> addDrawCallStep(RenderPass pass = PREPARE, bool fixed = false);

		template<typename T>
		void freeOnThread(Ref<T> ref) {
			refFreeList.push_back((Ref<void>)ref);
		}

		void freeOnThread(std::function<void()> callback) {
			callbackFreeList.push_back(callback);
		}

	private:
		std::vector<Ref<Step>> steps;
		std::vector<Ref<Step>> preparedSteps;
		std::vector<Ref<void>> refFreeList;
		std::vector<std::function<void()>> callbackFreeList;
		std::mutex mutex;
		RenderSettings::Statistics statistics;
		bool processedPrepared;
	};

}
