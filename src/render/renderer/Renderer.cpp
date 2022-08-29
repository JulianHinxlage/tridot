//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Renderer.h"
#include "entity/World.h"
#include "engine/AssetManager.h"
#include "engine/MeshComponent.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include "engine/Light.h"
#include "window/RenderContext.h"
#include "window/Viewport.h"
#include "RenderPipeline.h"
#include "engine/RuntimeMode.h"
#include <GL/glew.h>
#include <tracy/TracyOpenGL.hpp>

namespace tri {

    TRI_SYSTEM(Renderer);

    void Renderer::init() {
        auto* job = env->jobManager->addJob("Renderer");
        job->addSystem<Renderer>();
        env->runtimeMode->setActiveSystem(RuntimeMode::EDIT, "Renderer", true);
        env->runtimeMode->setActiveSystem(RuntimeMode::PAUSED, "Renderer", true);
    }

    void Renderer::startup() {
        setupSpecs();

        env->renderPipeline->addCallbackStep([&]() {
            Image image;
            image.init(1, 1, 4, 8);
            image.set(0, 0, color::white);
            defaultTexture = Ref<Texture>::make();
            defaultTexture->load(image);
            defaultMaterial = Ref<Material>::make();
            defaultMesh = Ref<Mesh>::make();
            float quadVertices[] = {
                -0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0,
                +0.5, +0.0, -0.5, 0.0, 1.0, 0.0, 1.0, 0.0,
                +0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 1.0, 1.0,
                -0.5, +0.0, +0.5, 0.0, 1.0, 0.0, 0.0, 1.0,
            };
            int quadIndices[] = {
                0, 2, 1,
                0, 3, 2,
            };
            defaultMesh->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });

            envBuffer = Ref<Buffer>::make();
            envBuffer->init(nullptr, 0, sizeof(envData), BufferType::UNIFORM_BUFFER, true);


            geometryShader = env->assetManager->get<Shader>("shaders/geometry.glsl");
            ambientLightShader = env->assetManager->get<Shader>("shaders/ambientLight.glsl");
            directionalLightShader = env->assetManager->get<Shader>("shaders/directionalLight.glsl");
            pointLightShader = env->assetManager->get<Shader>("shaders/pointLight.glsl");
            sphereMesh = env->assetManager->get<Mesh>("models/sphere.obj");
        });
    }

    void Renderer::shutdown() {
        env->renderPipeline->freeOnThread(defaultTexture);
        env->renderPipeline->freeOnThread(defaultMaterial);
        env->renderPipeline->freeOnThread(defaultMesh);
        env->renderPipeline->freeOnThread(geometryShader);
        env->renderPipeline->freeOnThread(ambientLightShader);
        env->renderPipeline->freeOnThread(directionalLightShader);
        env->renderPipeline->freeOnThread(pointLightShader);
        env->renderPipeline->freeOnThread(envBuffer);
        env->renderPipeline->freeOnThread(gBuffer);
        env->renderPipeline->freeOnThread(lightAccumulationBuffer);
        env->renderPipeline->freeOnThread(sphereMesh);

        defaultTexture = nullptr;
        defaultMaterial = nullptr;
        defaultMesh = nullptr;
        geometryShader = nullptr;
        ambientLightShader = nullptr;
        directionalLightShader = nullptr;
        pointLightShader = nullptr;
        envBuffer = nullptr;
        gBuffer = nullptr;
        lightAccumulationBuffer = nullptr;
        sphereMesh = nullptr;

        drawList.reset();

        for (auto& i : batches.batches) {
            for (auto& j : i.second) {
                env->renderPipeline->freeOnThread(j.second);
            }
        }
        batches.clear();
    }

    void Renderer::tick() {
        if (!geometryShader) {
            return;
        }

        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::OPAQUE);

        if (!updateFrameBuffer(gBuffer, gBufferSpec)) {
            return;
        }
        if (!updateFrameBuffer(lightAccumulationBuffer, lightAccumulationSpec)) {
            return;
        }

        bool hasPrimary = false;
        env->world->each<Camera>([&](Camera& camera) {
            if (camera.active) {
                if (env->viewport->size.y != 0) {
                    camera.aspectRatio = (float)env->viewport->size.x / (float)env->viewport->size.y;
                }

                Transform eye;
                eye.decompose(camera.transform);
                eyePosition = eye.position;

                submitMeshes();
                submitBatches(camera, gBuffer.get());
                env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::LIGHTING);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_FRONT, RenderPipeline::LIGHTING);
                submitLights(camera);
                camera.output = lightAccumulationBuffer;
                env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::LIGHTING);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_BACK, RenderPipeline::LIGHTING);

                if (camera.isPrimary && !hasPrimary) {
                    env->renderPipeline->freeOnThread(env->viewport->frameBuffer);
                    env->viewport->frameBuffer = camera.output;
                    hasPrimary = true;
                }
            }
        });
    }

    void Renderer::submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color, EntityId id) {
        if (!mesh) {
            mesh = defaultMesh.get();
        }
        if (!material) {
            material = defaultMaterial.get();
        }
        Shader* shader = material->shader.get();
        if (!shader) {
            shader = geometryShader.get();
        }
        if (shader->getId() == 0) {
            return;
        }
        if (mesh->vertexArray.getId() == 0) {
            return;
        }

        DrawList::Entry entry;
        entry.shader = shader;
        entry.mesh = mesh;
        entry.transform = transform;
        entry.material = material;
        entry.color = color.vec();
        entry.id = id;
        drawList.add(entry);
    }

    bool Renderer::updateFrameBuffer(Ref<FrameBuffer>& frameBuffer, const std::vector<FrameBufferAttachmentSpec>& spec) {
        TRI_PROFILE_FUNC();
        if (frameBuffer == nullptr) {
            env->renderPipeline->freeOnThread(frameBuffer);
            env->renderPipeline->addCallbackStep([&]() {
                frameBuffer = Ref<FrameBuffer>::make();
                frameBuffer->init(0, 0, spec);
                frameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
            });
            return false;
        }
        else {
            if (env->viewport->size != glm::ivec2(frameBuffer->getSize())) {
                env->renderPipeline->addCallbackStep([frameBuffer = frameBuffer]() {
                    TracyGpuZone("resize");
                    frameBuffer->resize(env->viewport->size.x, env->viewport->size.y);
                });
            }
            env->renderPipeline->addCallbackStep([frameBuffer = frameBuffer]() {
                TracyGpuZone("clear");
                frameBuffer->clear();
            });
            return true;
        }
    }

    void Renderer::submitMeshes() {
        TRI_PROFILE_FUNC();
        env->world->each<const Transform, const MeshComponent>([&](EntityId id, const Transform& t, const MeshComponent& m) {
            submit(t.getMatrix(), m.mesh.get(), m.material.get(), m.color, id);
        });
    }

    void Renderer::submitBatches(Camera &c, FrameBuffer *frameBuffer) {
        TRI_PROFILE_FUNC();
        envData.projection = c.projection;
        envData.viewProjection = c.viewProjection;
        envData.view = c.view;
        envData.eyePosition = eyePosition;
        envData.lightCount = 0;
        envData.radianceMapIndex = -1;
        envData.irradianceMapIndex = -1;
        env->renderPipeline->addCallbackStep([&]() {
            envBuffer->setData(&envData, sizeof(envData));
        });

        drawList.sort();
        drawList.submit(batches);
        drawList.reset();
        for (auto& i : batches.batches) {
            for (auto& j : i.second) {
                if (j.second->isInitialized()) {
                    j.second->environmentBuffer = envBuffer;
                    j.second->defaultTexture = defaultTexture.get();
                    j.second->submit(frameBuffer, RenderPipeline::OPAQUE);
                    j.second->reset();
                }
            }
        }
    }

    void Renderer::setupSpecs() {
        FrameBufferAttachmentSpec albedo;
        albedo.type = (TextureAttachment)(COLOR);
        albedo.clearColor = color::transparent;
        albedo.mipMapping = false;
        albedo.name = "Albedo";
        albedo.textureFormat = TextureFormat::RGBA8;

        FrameBufferAttachmentSpec id;
        id.type = (TextureAttachment)(COLOR + 1);
        id.clearColor = color::white;
        id.mipMapping = false;
        id.name = "ID";

        FrameBufferAttachmentSpec normal;
        normal.type = (TextureAttachment)(COLOR + 2);
        normal.clearColor = color::black;
        normal.name = "Normal";
        normal.textureFormat = TextureFormat::RGB8;

        FrameBufferAttachmentSpec position;
        position.type = (TextureAttachment)(COLOR + 3);
        position.clearColor = color::black;
        position.name = "Position";
        position.textureFormat = TextureFormat::RGB32F;
        position.sRepeat = false;
        position.tRepeat = false;
        position.useBorderColor = true;
        position.borderColor = color::black;

        FrameBufferAttachmentSpec rme;
        rme.type = (TextureAttachment)(COLOR + 4);
        rme.clearColor = color::black;
        rme.name = "RME";
        rme.textureFormat = TextureFormat::RGB8;

        FrameBufferAttachmentSpec depth;
        depth.type = (TextureAttachment)(DEPTH);
        depth.clearColor = color::white;
        depth.name = "Depth";
        depth.textureFormat = TextureFormat::DEPTH24STENCIL8;

        gBufferSpec = { albedo, id, normal, position, rme, depth };

        FrameBufferAttachmentSpec light;
        light.type = (TextureAttachment)(COLOR);
        light.clearColor = color::black;
        light.mipMapping = false;
        light.name = "Light";
        light.textureFormat = TextureFormat::RGBA8;

        lightAccumulationSpec = { light, id};
    }

    void Renderer::submitLights(const Camera& camera) {
        TRI_PROFILE_FUNC();

        bool hasLight = false;
        env->world->each<const Transform, const Light>([&](EntityId id, const Transform& t, const Light& light) {
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        });

        if (!hasLight) {
            Light light;
            light.type = Light::AMBIENT_LIGHT;
            light.intensity = 1.0f;
            light.color = color::white;
            Transform t;
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        }
    }
    
    bool Renderer::submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const Light& light, const Transform &transform, const Camera& camera) {
        TRI_PROFILE_FUNC();
        switch (light.type) {
        case Light::AMBIENT_LIGHT: {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);

            dc->vertexArray = &defaultMesh->vertexArray;
            dc->shader = ambientLightShader.get();
            dc->frameBuffer = lightAccumulationBuffer.get();
            dc->images.push_back(lightAccumulationBuffer->getAttachment(TextureAttachment::COLOR).get());
            dc->textures.push_back(nullptr);
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 4)).get());
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("uColor", light.color.vec());
            dc->shaderState->set("uIntesity", light.intensity);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
            dc->shaderState->set("uLightBuffer", 0);
            return true;
        }
        case Light::DIRECTIONAL_LIGHT: {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);

            dc->vertexArray = &defaultMesh->vertexArray;
            dc->shader = directionalLightShader.get();
            dc->frameBuffer = lightAccumulationBuffer.get();
            dc->images.push_back(lightAccumulationBuffer->getAttachment(TextureAttachment::COLOR).get());
            dc->textures.push_back(nullptr);
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 4)).get());
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("uColor", light.color.vec());
            dc->shaderState->set("uIntesity", light.intensity);

            Transform directionTransform;
            directionTransform.rotation = transform.rotation;
            glm::vec3 direction = directionTransform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 1);
            dc->shaderState->set("uDirection", direction);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());

            dc->shaderState->set("uEyePosition", eyePosition);
            dc->shaderState->set("uLightBuffer", 0);
            return true;
        }
        case Light::POINT_LIGHT: {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);

            dc->vertexArray = &sphereMesh->vertexArray;
            dc->shader = pointLightShader.get();
            dc->frameBuffer = lightAccumulationBuffer.get();
            dc->images.push_back(lightAccumulationBuffer->getAttachment(TextureAttachment::COLOR).get());

            dc->textures.push_back(nullptr);
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 4)).get());
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());

            Transform positionTransform;
            positionTransform.position = transform.position;
            positionTransform.decompose(transform.getMatrix());

            Transform sphereTransform;
            sphereTransform.position = positionTransform.position;
            sphereTransform.scale = glm::vec3(2, 2, 2) *  light.range;
            glm::mat sphereMatrix = camera.viewProjection * sphereTransform.calculateLocalMatrix();

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", sphereMatrix);
            dc->shaderState->set("uColor", light.color.vec());
            dc->shaderState->set("uIntesity", light.intensity);
            dc->shaderState->set("uRange", light.range);
            dc->shaderState->set("uFalloff", light.falloff);
            dc->shaderState->set("uPosition", positionTransform.position);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());

            dc->shaderState->set("uEyePosition", eyePosition);
            dc->shaderState->set("uLightBuffer", 0);
            return true;
        }
        case Light::SPOT_LIGHT: {
            return false;
        }
        default:
            return false;
        }
    }

}
