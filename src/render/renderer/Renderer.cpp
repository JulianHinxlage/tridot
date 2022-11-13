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
#include "engine/Skybox.h"
#include "window/RenderContext.h"
#include "window/Viewport.h"
#include "RenderPipeline.h"
#include "MeshFactory.h"
#include "engine/Random.h"
#include <GL/glew.h>
#include <tracy/TracyOpenGL.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace tri {

    TRI_SYSTEM(Renderer);

    void Renderer::init() {
        env->jobManager->addJob("Renderer", {"Renderer"});
    }

    float lerp(float x, float y, float t) {
        return x * (1.0f - t) + y * t;
    }

    void Renderer::startup() {
        setupSpecs();

        geometryShader = env->assetManager->get<Shader>("shaders/geometry.glsl");
        ambientLightShader = env->assetManager->get<Shader>("shaders/ambientLight.glsl");
        directionalLightShader = env->assetManager->get<Shader>("shaders/directionalLight.glsl");
        pointLightShader = env->assetManager->get<Shader>("shaders/pointLight.glsl");
        spotLightShader = env->assetManager->get<Shader>("shaders/spotLight.glsl");
        bloomShader = env->assetManager->get<Shader>("shaders/bloom.glsl");
        blurShader = env->assetManager->get<Shader>("shaders/gaussianBlur.glsl");
        compositShader = env->assetManager->get<Shader>("shaders/composit.glsl");
        skyboxShader = env->assetManager->get<Shader>("shaders/skybox.glsl");
        ssaoShader = env->assetManager->get<Shader>("shaders/ssao.glsl");
        shadowShader = env->assetManager->get<Shader>("shaders/mesh.glsl");
        colorGradingShader = env->assetManager->get<Shader>("shaders/colorGrading.glsl");
        coneMesh = env->assetManager->get<Mesh>("models/cone.obj");
        cubeMesh = env->assetManager->get<Mesh>("models/cube.obj");

        env->renderPipeline->addCallbackStep([&]() {
            Image image;
            image.init(1, 1, 4, 8);
            image.set(0, 0, color::white);
            defaultTexture = Ref<Texture>::make();
            defaultTexture->load(image);
            defaultMaterial = Ref<Material>::make();

            quadMesh = env->systemManager->getSystem<MeshFactory>()->generateQuad();
            sphereMesh = env->systemManager->getSystem<MeshFactory>()->generateCubeSphere(3);

            envBuffer = Ref<Buffer>::make();
            envBuffer->init(nullptr, 0, sizeof(envData), BufferType::UNIFORM_BUFFER, true);

            shadowEnvBuffer = Ref<Buffer>::make();
            shadowEnvBuffer->init(nullptr, 0, sizeof(shadowEnvData), BufferType::UNIFORM_BUFFER, true);

            pointLightBatch.instanceBuffer = Ref<BatchBuffer>::make();
            pointLightBatch.instanceBuffer->init(sizeof(LightBatch::Instance));
        
            spotLightBatch.instanceBuffer = Ref<BatchBuffer>::make();
            spotLightBatch.instanceBuffer->init(sizeof(LightBatch::Instance));

            int noiseResolution = 4;
            std::vector<Color> noiseData;
            noiseData.resize(noiseResolution * noiseResolution);
            for (int i = 0; i < noiseData.size(); i++) {
                noiseData[i] = Color(glm::vec3(env->random->getVec2(), 0.0));
            }

            Image noiseImage;
            noiseImage.init(noiseResolution, noiseResolution, 3, 8, noiseData.data(), noiseData.size());
            ssaoNoise = Ref<Texture>::make();
            ssaoNoise->load(noiseImage);
        });
    }

    void Renderer::shutdown() {
        env->renderPipeline->freeOnThread(defaultTexture);
        env->renderPipeline->freeOnThread(defaultMaterial);
        env->renderPipeline->freeOnThread(quadMesh);
        env->renderPipeline->freeOnThread(geometryShader);
        env->renderPipeline->freeOnThread(ambientLightShader);
        env->renderPipeline->freeOnThread(directionalLightShader);
        env->renderPipeline->freeOnThread(pointLightShader);
        env->renderPipeline->freeOnThread(spotLightShader);
        env->renderPipeline->freeOnThread(bloomShader);
        env->renderPipeline->freeOnThread(blurShader);
        env->renderPipeline->freeOnThread(compositShader);
        env->renderPipeline->freeOnThread(skyboxShader);
        env->renderPipeline->freeOnThread(ssaoShader);
        env->renderPipeline->freeOnThread(envBuffer);
        env->renderPipeline->freeOnThread(gBuffer);
        env->renderPipeline->freeOnThread(lightAccumulationBuffer);
        env->renderPipeline->freeOnThread(transparencyBuffer);
        env->renderPipeline->freeOnThread(bloomBuffer1);
        env->renderPipeline->freeOnThread(bloomBuffer2);
        env->renderPipeline->freeOnThread(ssaoBuffer);
        env->renderPipeline->freeOnThread(ssaoNoise);
        env->renderPipeline->freeOnThread(sphereMesh);
        env->renderPipeline->freeOnThread(coneMesh);
        env->renderPipeline->freeOnThread(cubeMesh);
        env->renderPipeline->freeOnThread(pointLightBatch.vertexArray);
        env->renderPipeline->freeOnThread(pointLightBatch.instanceBuffer);
        env->renderPipeline->freeOnThread(spotLightBatch.vertexArray);
        env->renderPipeline->freeOnThread(spotLightBatch.instanceBuffer);
        env->renderPipeline->freeOnThread(shadowEnvBuffer);
        env->renderPipeline->freeOnThread(shadowShader);
        env->renderPipeline->freeOnThread(colorGradingShader);
        env->renderPipeline->freeOnThread(postProcessingBuffer);

        defaultTexture = nullptr;
        defaultMaterial = nullptr;
        quadMesh = nullptr;
        geometryShader = nullptr;
        ambientLightShader = nullptr;
        directionalLightShader = nullptr;
        pointLightShader = nullptr;
        spotLightShader = nullptr;
        bloomShader = nullptr;
        blurShader = nullptr;
        skyboxShader = nullptr;
        compositShader = nullptr;
        ssaoShader = nullptr;
        envBuffer = nullptr;
        gBuffer = nullptr;
        lightAccumulationBuffer = nullptr;
        transparencyBuffer = nullptr;
        bloomBuffer1 = nullptr;
        bloomBuffer2 = nullptr;
        ssaoBuffer = nullptr;
        ssaoNoise = nullptr;
        sphereMesh = nullptr;
        coneMesh = nullptr;
        cubeMesh = nullptr;
        pointLightBatch.vertexArray = nullptr;
        pointLightBatch.instanceBuffer = nullptr;
        spotLightBatch.vertexArray = nullptr;
        spotLightBatch.instanceBuffer = nullptr;
        shadowShader = nullptr;
        shadowEnvBuffer = nullptr;
        colorGradingShader = nullptr;
        postProcessingBuffer = nullptr;

        drawList.reset();
        transparencyDrawList.reset();
        batches.clear();
        transparencyBatches.clear();
        shadowBatches.clear();
    }

    void Renderer::tick() {
        updateFrameBuffer(gBuffer, gBufferSpec, env->viewport->size);
        updateFrameBuffer(lightAccumulationBuffer, lightAccumulationSpec, env->viewport->size);
        updateFrameBuffer(postProcessingBuffer, lightAccumulationSpec, env->viewport->size);
        if (env->renderSettings->enableBloom) {
            updateFrameBuffer(bloomBuffer1, bloomBufferSpec, env->viewport->size);
            updateFrameBuffer(bloomBuffer2, bloomBufferSpec, env->viewport->size);
        }
        if (env->renderSettings->enableSSAO) {
            updateFrameBuffer(ssaoBuffer, ssaoBufferSpec, env->viewport->size);
        }

        prepareTransparencyBuffer();
        prepareLightBatches();

        if (!defaultMaterial) {
            return;
        }
        if (!gBuffer) {
            return;
        }

        submitShadows();

        bool hasPrimary = false;
        env->world->each<Camera>([&](Camera& camera) {
            if (camera.active) {
                if (env->viewport->size.y != 0) {
                    camera.aspectRatio = (float)env->viewport->size.x / (float)env->viewport->size.y;
                }

                Transform eye;
                eye.decompose(camera.transform);
                eyePosition = eye.position;
                drawList.eyePosition = eyePosition;
                transparencyDrawList.eyePosition = eyePosition;
                
                frustum.viewProjectionMatrix = camera.viewProjection;

                submitSkyBox(camera);

                submitMeshes();

                submitBatches(camera);

                if (env->renderSettings->enableSSAO) {
                    submitSSAO();
                }

                submitLights(camera);

                if (env->renderSettings->enablePointLights) {
                    submitPointLightBatch();
                }
                if (env->renderSettings->enableSpotLights) {
                    submitSpotLightBatch();
                }
                if (env->renderSettings->enableBloom) {
                    submitBloom();
                }

                if (env->renderSettings->enableColorGrading) {
                    submitPostProcessing();
                    camera.output = postProcessingBuffer;
                }
                else {
                    camera.output = lightAccumulationBuffer;
                }

                env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::LIGHTING);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_BACK, RenderPipeline::LIGHTING);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_ON, RenderPipeline::LIGHTING);

                if (camera.isPrimary && !hasPrimary) {
                    env->renderPipeline->freeOnThread(env->viewport->frameBuffer);
                    env->viewport->frameBuffer = camera.output;
                    env->viewport->idMap = gBuffer->getAttachment("ID");
                    hasPrimary = true;
                }
            }
        });

        if (env->viewport->frameBuffer && env->viewport->displayInWindow) {
            submitDisplay();
        }
    }

    void Renderer::submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color, EntityId id) {
        if (!mesh) {
            mesh = quadMesh.get();
        }

        if (env->renderSettings->enableFrustumCulling && !frustum.inFrustum(transform, mesh)) {
            return;
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

        bool opaque = entry.color.a == 255 && entry.material->color.a == 255;
        if (opaque) {
            drawList.add(entry);
        }
        else {
            transparencyDrawList.add(entry);
        }
    }

    Ref<FrameBuffer> &Renderer::getGBuffer() {
        return gBuffer;
    }

    Ref<FrameBuffer>& Renderer::getBloomBuffer() {
        return bloomBuffer1;
    }

    void Renderer::prepareTransparencyBuffer() {
        if (!transparencyBuffer && gBuffer && lightAccumulationBuffer) {
            env->renderPipeline->addCallbackStep([&]() {
                transparencyBuffer = Ref<FrameBuffer>::make();
                transparencyBuffer->init(0, 0, {});
            });
        }
        if (transparencyBuffer) {
            env->renderPipeline->addCallbackStep([&]() {
                transparencyBuffer->setAttachment((FrameBufferAttachmentSpec)(TextureAttachment)(TextureAttachment::COLOR + 0), lightAccumulationBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 0)));
                transparencyBuffer->setAttachment((FrameBufferAttachmentSpec)(TextureAttachment)(TextureAttachment::COLOR + 1), gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)));
                transparencyBuffer->setAttachment((FrameBufferAttachmentSpec)(TextureAttachment)(TextureAttachment::COLOR + 2), gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)));
                transparencyBuffer->setAttachment((FrameBufferAttachmentSpec)(TextureAttachment)(TextureAttachment::COLOR + 3), gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)));
                transparencyBuffer->setAttachment((FrameBufferAttachmentSpec)(TextureAttachment)(TextureAttachment::COLOR + 4), gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 4)));
                transparencyBuffer->setAttachment((FrameBufferAttachmentSpec)(TextureAttachment)(TextureAttachment::DEPTH), gBuffer->getAttachment((TextureAttachment)(TextureAttachment::DEPTH)));
            });
        }
    }

    bool Renderer::updateFrameBuffer(Ref<FrameBuffer>& frameBuffer, const std::vector<FrameBufferAttachmentSpec>& spec, glm::vec2 size) {
        TRI_PROFILE_FUNC();
        if (frameBuffer == nullptr) {
            env->renderPipeline->freeOnThread(frameBuffer);
            env->renderPipeline->addCallbackStep([&]() {
                frameBuffer = Ref<FrameBuffer>::make();
                frameBuffer->init(0, 0, spec);
            });
            return false;
        }
        else {
            if (size != frameBuffer->getSize()) {
                env->renderPipeline->addCallbackStep([size, frameBuffer = frameBuffer]() {
                    TracyGpuZone("resize");
                    frameBuffer->resize(size.x, size.y);
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

    void Renderer::submitBatches(Camera &c) {
        TRI_PROFILE_FUNC();
        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::GEOMETRY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_BACK, RenderPipeline::GEOMETRY);

        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::TRANSPARENCY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_BACK, RenderPipeline::TRANSPARENCY);

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
                    j.second->submit(gBuffer.get(), RenderPipeline::GEOMETRY);
                    j.second->reset();
                }
            }
        }

        if (env->renderSettings->enableTransparency) {
            transparencyDrawList.sort();
            transparencyDrawList.submit(transparencyBatches);
            transparencyDrawList.reset();
            for (auto& i : transparencyBatches.batches) {
                for (auto& j : i.second) {
                    if (j.second->isInitialized()) {
                        j.second->environmentBuffer = envBuffer;
                        j.second->defaultTexture = defaultTexture.get();
                        j.second->submit(transparencyBuffer.get(), RenderPipeline::TRANSPARENCY);
                        j.second->reset();
                    }
                }
            }
        }
        else {
            transparencyDrawList.reset();
        }
    }

    void Renderer::setupSpecs() {
        FrameBufferAttachmentSpec albedo;
        albedo.type = (TextureAttachment)(COLOR);
        albedo.clearColor = color::transparent;
        albedo.mipMapping = false;
        albedo.name = "Albedo";
        albedo.textureFormat = TextureFormat::RGBA8;

        FrameBufferAttachmentSpec normal;
        normal.type = (TextureAttachment)(COLOR + 1);
        normal.clearColor = color::black;
        normal.name = "Normal";
        normal.textureFormat = TextureFormat::RGB8;

        FrameBufferAttachmentSpec position;
        position.type = (TextureAttachment)(COLOR + 2);
        position.clearColor = color::black;
        position.name = "Position";
        position.textureFormat = TextureFormat::RGB32F;
        position.sRepeat = false;
        position.tRepeat = false;
        position.useBorderColor = true;
        position.borderColor = color::black;

        FrameBufferAttachmentSpec rme;
        rme.type = (TextureAttachment)(COLOR + 3);
        rme.clearColor = color::black;
        rme.name = "RME";
        rme.textureFormat = TextureFormat::RGB8;

        FrameBufferAttachmentSpec id;
        id.type = (TextureAttachment)(COLOR + 4);
        id.clearColor = color::white;
        id.mipMapping = false;
        id.name = "ID";
        
        FrameBufferAttachmentSpec depth;
        depth.type = (TextureAttachment)(DEPTH);
        depth.clearColor = color::white;
        depth.name = "Depth";
        depth.textureFormat = TextureFormat::DEPTH24STENCIL8;

        gBufferSpec = { albedo, normal, position, rme, id, depth };


        FrameBufferAttachmentSpec light;
        light.type = (TextureAttachment)(COLOR);
        light.clearColor = color::black;
        light.mipMapping = false;
        light.name = "Light";
        light.textureFormat = TextureFormat::RGBA16F;

        lightAccumulationSpec = { light };


        FrameBufferAttachmentSpec bloom;
        bloom.type = (TextureAttachment)(COLOR);
        bloom.clearColor = color::black;
        bloom.mipMapping = false;
        bloom.name = "Bloom";
        bloom.textureFormat = TextureFormat::RGBA8;

        bloomBufferSpec = { bloom };


        FrameBufferAttachmentSpec ssao;
        ssao.type = (TextureAttachment)(COLOR);
        ssao.clearColor = color::white;
        ssao.mipMapping = false;
        ssao.name = "SSAO";
        ssao.textureFormat = TextureFormat::RED8;
        ssao.magNearest = true;
        ssao.minNearest = true;

        ssaoBufferSpec = { ssao };

        FrameBufferAttachmentSpec shadow;
        shadow.type = (TextureAttachment)(DEPTH);
        shadow.clearColor = color::white;
        shadow.name = "Shadows";
        shadow.textureFormat = TextureFormat::DEPTH32;
        shadow.magNearest = false;
        shadow.minNearest = false;
        shadow.sRepeat = false;
        shadow.tRepeat = false;

        shadowMapSpec = { shadow };
    }

    bool Renderer::prepareLightBatches() {
        bool needsPrepare = false;
        if (!pointLightBatch.vertexArray && !pointLightBatch.hasPrepared) {
            if (sphereMesh && sphereMesh->vertexArray.getId() != 0) {
                TRI_PROFILE("add prepare point light batch");
                env->renderPipeline->addCallbackStep([&]() {
                    TRI_PROFILE("prepare point light batch");
                    pointLightBatch.vertexArray = Ref<VertexArray>::make(sphereMesh->vertexArray);
                    pointLightBatch.vertexArray->addVertexBuffer(pointLightBatch.instanceBuffer->buffer, {
                        //transform
                        {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4},
                        //position
                        {FLOAT, 3},
                        //direction
                        {FLOAT, 3},
                        //color
                        {UINT8, 4, true},
                        //intensity
                        {FLOAT, 1},
                        //range
                        {FLOAT, 1},
                        //falloff
                        {FLOAT, 1},
                        //spot angle
                        {FLOAT, 1},
                        }, 1);
                });
                pointLightBatch.hasPrepared = true;
                needsPrepare = true;
            }
        }
        if (!spotLightBatch.vertexArray && !spotLightBatch.hasPrepared) {
            if (coneMesh && coneMesh->vertexArray.getId() != 0) {
                TRI_PROFILE("add prepare spot light batch");
                env->renderPipeline->addCallbackStep([&]() {
                    TRI_PROFILE("prepare spot light batch");
                    spotLightBatch.vertexArray = Ref<VertexArray>::make(coneMesh->vertexArray);
                    spotLightBatch.vertexArray->addVertexBuffer(spotLightBatch.instanceBuffer->buffer, {
                        //transform
                        {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4},
                        //position
                        {FLOAT, 3},
                        //direction
                        {FLOAT, 3},
                        //color
                        {UINT8, 4, true},
                        //intensity
                        {FLOAT, 1},
                        //range
                        {FLOAT, 1},
                        //falloff
                        {FLOAT, 1},
                        //spot angle
                        {FLOAT, 1},
                        }, 1);
                });
                spotLightBatch.hasPrepared = true;
                needsPrepare = true;
            }
        }
        return !needsPrepare;
    }

    void Renderer::submitPointLightBatch(){
        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
        dc->name = "point lights";

        dc->vertexArray = pointLightBatch.vertexArray.get();
        dc->instanceCount = pointLightBatch.instanceBuffer->size();
        dc->buffers.push_back(pointLightBatch.instanceBuffer.get());
        pointLightBatch.instanceBuffer->swapBuffers();

        dc->shader = pointLightShader.get();
        dc->frameBuffer = lightAccumulationBuffer.get();

        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());

        dc->shaderState = Ref<ShaderState>::make();
        
        std::vector<int> textureSlots;
        for (int i = 0; i < dc->textures.size(); i++) {
            textureSlots.push_back(i);
        }
        dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        dc->shaderState->set("uEyePosition", eyePosition);
    }

    void Renderer::submitSpotLightBatch() {
        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
        dc->name = "spot lights";

        dc->vertexArray = spotLightBatch.vertexArray.get();
        dc->instanceCount = spotLightBatch.instanceBuffer->size();
        dc->buffers.push_back(spotLightBatch.instanceBuffer.get());
        spotLightBatch.instanceBuffer->swapBuffers();

        dc->shader = spotLightShader.get();
        dc->frameBuffer = lightAccumulationBuffer.get();

        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());

        dc->shaderState = Ref<ShaderState>::make();

        std::vector<int> textureSlots;
        for (int i = 0; i < dc->textures.size(); i++) {
            textureSlots.push_back(i);
        }
        dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        dc->shaderState->set("uEyePosition", eyePosition);
    }

    void Renderer::submitLights(const Camera& camera) {
        TRI_PROFILE_FUNC();

        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::LIGHTING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_FRONT, RenderPipeline::LIGHTING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_ADDITIVE, RenderPipeline::LIGHTING);

        bool hasLight = false;
        env->world->each<const Transform, const AmbientLight>([&](EntityId id, const Transform& t, const AmbientLight& light) {
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        });
        env->world->each<const Transform, const DirectionalLight>([&](EntityId id, const Transform& t, const DirectionalLight& light) {
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        });
        env->world->each<const Transform, const PointLight>([&](EntityId id, const Transform& t, const PointLight& light) {
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        });
        env->world->each<const Transform, const SpotLight>([&](EntityId id, const Transform& t, const SpotLight& light) {
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        });

        if (!hasLight) {
            AmbientLight light;
            light.intensity = 1.0f;
            light.color = color::white;
            Transform t;
            if (submitLight(lightAccumulationBuffer.get(), gBuffer.get(), light, t, camera)) {
                hasLight = true;
            }
        }
    }
    
    bool Renderer::submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const AmbientLight& light, const Transform& transform, const Camera& camera) {
        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
        dc->name = "ambient light";

        dc->vertexArray = &quadMesh->vertexArray;
        dc->shader = ambientLightShader.get();
        dc->frameBuffer = lightAccumulationBuffer.get();
        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());
        if (env->renderSettings->enableSSAO) {
            dc->textures.push_back(ssaoBuffer->getAttachment(TextureAttachment::COLOR).get());
        }
        else {
            dc->textures.push_back(defaultTexture.get());
        }

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
        return true;
    }
    
    bool Renderer::submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const DirectionalLight& light, const Transform& transform, const Camera& camera) {
        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
        dc->name = "directional light";

        dc->vertexArray = &quadMesh->vertexArray;
        dc->shader = directionalLightShader.get();
        dc->frameBuffer = lightAccumulationBuffer.get();
        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
        dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
        dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());

        if (light.shadows && light.shadowMap && env->renderSettings->enableShadows) {
            dc->textures.push_back(light.shadowMap->getAttachment(TextureAttachment::DEPTH).get());
        }
        else {
            dc->textures.push_back(defaultTexture.get());
        }

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

        float near = 1.0f;
        float far = 100.0f;
        float size = 32.0f;
        glm::mat4 projection = glm::ortho(-size, size, -size, size, near, far);
        glm::mat4 view = glm::lookAt(transform.position - direction, transform.position, { 0, 1, 0 });
        glm::mat4 viewProjection = projection * view;
        dc->shaderState->set("uLightProjection", viewProjection);

        std::vector<int> textureSlots;
        for (int i = 0; i < dc->textures.size(); i++) {
            textureSlots.push_back(i);
        }
        dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        dc->shaderState->set("uEyePosition", eyePosition);
        return true;
    }

    bool Renderer::submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const PointLight& light, const Transform& transform, const Camera& camera) {
        Transform positionTransform;
        positionTransform.decompose(transform.getMatrix());

        Transform sphereTransform;
        sphereTransform.position = positionTransform.position;
        sphereTransform.scale = glm::vec3(1.1, 1.1, 1.1) * light.range;
        glm::mat4 sphereMatrix = sphereTransform.calculateLocalMatrix();

        Transform directionTransform;
        directionTransform.rotation = transform.rotation;
        glm::vec3 direction = directionTransform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 1);

        if (env->renderSettings->enableFrustumCulling && !frustum.inFrustum(sphereMatrix, sphereMesh.get())) {
            return true;
        }

        sphereMatrix = camera.viewProjection * sphereMatrix;

        LightBatch::Instance* iData = (LightBatch::Instance*)pointLightBatch.instanceBuffer->next();
        iData->transform = sphereMatrix;
        iData->position = positionTransform.position;
        iData->direction = direction;
        iData->color = light.color;
        iData->intensity = light.intensity;
        iData->range = light.range;
        iData->falloff = light.falloff;
        iData->spotAngle = 0;
        return true;
    }

    bool Renderer::submitLight(FrameBuffer* lightBuffer, FrameBuffer* gBuffer, const SpotLight& light, const Transform& transform, const Camera& camera) {
        LightBatch::Instance* iData = (LightBatch::Instance*)spotLightBatch.instanceBuffer->next();

        Transform positionTransform;
        positionTransform.decompose(transform.getMatrix());

        Transform directionTransform;
        directionTransform.rotation = positionTransform.rotation;
        glm::vec3 direction = directionTransform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 1);

        Transform coneTransform;
        coneTransform.position = positionTransform.position;
        float scale = glm::tan(glm::radians(light.spotAngle)) * 3.0f;
        coneTransform.scale = glm::vec3(1, scale, scale) * light.range;
        coneTransform.rotation = positionTransform.rotation;

        Transform offset;
        offset.position.x = -0.5f;
        offset.rotation.z = glm::radians(-90.0f);
        glm::mat4 coneMatrix = coneTransform.calculateLocalMatrix() * offset.calculateLocalMatrix();

        if (env->renderSettings->enableFrustumCulling && !frustum.inFrustum(coneMatrix, coneMesh.get())) {
            return true;
        }

        coneMatrix = camera.viewProjection * coneMatrix;

        iData->transform = coneMatrix;
        iData->position = positionTransform.position;
        iData->direction = direction;
        iData->color = light.color;
        iData->intensity = light.intensity;
        iData->range = light.range;
        iData->falloff = light.falloff;
        iData->spotAngle = glm::radians(light.spotAngle);
        return true;
    }

    void Renderer::submitBloom() {
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_OFF, RenderPipeline::LIGHTING);
        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "bloom";

            dc->vertexArray = &quadMesh->vertexArray;
            dc->shader = bloomShader.get();
            dc->frameBuffer = bloomBuffer1.get();
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::COLOR).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get());
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 3)).get());
            dc->textures.push_back(gBuffer->getAttachment(TextureAttachment::DEPTH).get());
            dc->textures.push_back(lightAccumulationBuffer->getAttachment(TextureAttachment::COLOR).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("bloomThreshold", env->renderSettings->bloomThreshold);
            
            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        }


        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "bloom vblur";

            dc->vertexArray = &quadMesh->vertexArray;
            dc->shader = blurShader.get();
            dc->frameBuffer = bloomBuffer2.get();
            dc->textures.push_back(bloomBuffer1->getAttachment(TextureAttachment::COLOR).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("spread", glm::vec2(1, 0));
            dc->shaderState->set("steps", env->renderSettings->bloomSpread);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        }

        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_ADDITIVE, RenderPipeline::LIGHTING);
        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "bloom hblur";

            dc->vertexArray = &quadMesh->vertexArray;
            dc->shader = blurShader.get();
            dc->frameBuffer = bloomBuffer1.get();
            dc->textures.push_back(bloomBuffer2->getAttachment(TextureAttachment::COLOR).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("spread", glm::vec2(0, 1));
            dc->shaderState->set("steps", env->renderSettings->bloomSpread);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        }

        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "bloom composit";

            dc->vertexArray = &quadMesh->vertexArray;
            dc->shader = compositShader.get();
            dc->frameBuffer = lightAccumulationBuffer.get();
            dc->textures.push_back(bloomBuffer1->getAttachment(TextureAttachment::COLOR).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("factor1", env->renderSettings->bloomIntesity);
            dc->shaderState->set("factor2", 0.0f);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        }

    }

    void Renderer::submitDisplay() {
        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::DISPLAY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_OFF, RenderPipeline::DISPLAY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_OFF, RenderPipeline::DISPLAY);

        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::DISPLAY);
        dc->name = "display";

        dc->vertexArray = &quadMesh->vertexArray;
        dc->shader = compositShader.get();
        dc->frameBuffer = nullptr;
        dc->textures.push_back(env->viewport->frameBuffer->getAttachment(TextureAttachment::COLOR).get());

        Transform quadTransform;
        quadTransform.rotation.x = glm::radians(90.0f);
        quadTransform.scale = { 2, 2, -2 };

        dc->shaderState = Ref<ShaderState>::make();
        dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
        dc->shaderState->set("uProjection", glm::mat4(1));

        dc->shaderState->set("factor1", 1.0f);
        dc->shaderState->set("factor2", 0.0f);

        std::vector<int> textureSlots;
        for (int i = 0; i < dc->textures.size(); i++) {
            textureSlots.push_back(i);
        }
        dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
    }

    void Renderer::submitSkyBox(const Camera &camera) {
        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::GEOMETRY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_OFF, RenderPipeline::GEOMETRY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_OFF, RenderPipeline::GEOMETRY);

        env->world->each<Skybox>([&](EntityId id, Skybox &skyBox) {
            if (skyBox.texture) {
                if (skyBox.texture->getType() != TextureType::TEXTURE_CUBE_MAP) {
                    env->renderPipeline->addCallbackStep([&]() {
                        skyBox.texture->setCubeMap(true);
                    });
                }

                auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::GEOMETRY);
                dc->shader = skyboxShader.get();
                dc->frameBuffer = gBuffer.get();
                dc->textures.push_back(skyBox.texture.get());
                dc->vertexArray = &cubeMesh->vertexArray;
                dc->shaderState = Ref<ShaderState>::make();
                dc->shaderState->set("uColor", skyBox.color.vec());

                Transform cameraTransform;
                cameraTransform.decompose(camera.transform);

                Transform cubeTransform;
                cubeTransform.scale = { 1, 1, 1 };
                cubeTransform.position = cameraTransform.position;
                if (auto* transform = env->world->getComponent<Transform>(id)) {
                    cubeTransform.rotation = transform->rotation;
                }

                dc->shaderState->set("uTransform", cubeTransform.calculateLocalMatrix());
                dc->shaderState->set("uProjection", camera.viewProjection);
            }
        });
    }

    void Renderer::submitSSAO() {
        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::LIGHTING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_OFF, RenderPipeline::LIGHTING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_OFF, RenderPipeline::LIGHTING);

        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "ssao";

            dc->frameBuffer = ssaoBuffer.get();
            dc->shader = ssaoShader.get();
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 2)).get()); //Position
            dc->textures.push_back(gBuffer->getAttachment((TextureAttachment)(TextureAttachment::COLOR + 1)).get()); //Normal
            dc->textures.push_back(ssaoNoise.get());
            dc->textures.push_back(gBuffer->getAttachment(DEPTH).get());

            dc->vertexArray = &quadMesh->vertexArray;

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uEnvironment", envBuffer.get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("uProjection", glm::mat4(1));

            dc->shaderState->set("kernalSize", env->renderSettings->ssaoKernalSize);
            dc->shaderState->set("sampleRadius", env->renderSettings->ssaoSampleRadius);
            dc->shaderState->set("bias", env->renderSettings->ssaoBias);
            dc->shaderState->set("occlusionStrength", env->renderSettings->ssaoOcclusionStrength);

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());

            if (env->renderSettings->ssaoKernalSize != ssaoSamples.size() && ssaoShader->getId() != 0) {
                //generating samples
                int kernalSize = env->renderSettings->ssaoKernalSize;
                ssaoSamples.resize(kernalSize);
                for (int i = 0; i < ssaoSamples.size(); i++) {
                    glm::vec3 sample = env->random->getVec3();
                    sample.x = sample.x * 2.0 - 1.0;
                    sample.y = sample.y * 2.0 - 1.0;
                    sample = glm::normalize(sample);
                    sample *= env->random->getFloat();

                    float scale = (float)i / (float)kernalSize;
                    scale = lerp(0.1f, 1.0f, scale * scale);
                    sample *= scale;
                    ssaoSamples[i] = sample;
                }

                dc->shaderState->set("samples", ssaoSamples.data(), ssaoSamples.size());
            }
        }
        
        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "ssao vblur";

            dc->vertexArray = &quadMesh->vertexArray;
            dc->shader = blurShader.get();
            dc->frameBuffer = bloomBuffer2.get();
            dc->textures.push_back(ssaoBuffer->getAttachment(TextureAttachment::COLOR).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("spread", glm::vec2(1, 0));
            dc->shaderState->set("steps", (int)ssaoNoise->getHeight());

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        }

        {
            auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::LIGHTING);
            dc->name = "ssao hblur";

            dc->vertexArray = &quadMesh->vertexArray;
            dc->shader = blurShader.get();
            dc->frameBuffer = ssaoBuffer.get();
            dc->textures.push_back(bloomBuffer2->getAttachment(TextureAttachment::COLOR).get());

            Transform quadTransform;
            quadTransform.rotation.x = glm::radians(90.0f);
            quadTransform.scale = { 2, 2, -2 };

            dc->shaderState = Ref<ShaderState>::make();
            dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
            dc->shaderState->set("spread", glm::vec2(0, 1));
            dc->shaderState->set("steps", (int)ssaoNoise->getWidth());

            std::vector<int> textureSlots;
            for (int i = 0; i < dc->textures.size(); i++) {
                textureSlots.push_back(i);
            }
            dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
        }
    }

    void Renderer::submitShadows() {
        if (env->renderSettings->enableShadows) {
            env->world->each<const Transform, DirectionalLight>([&](EntityId id, const Transform& transform, DirectionalLight& light) {
                if (light.shadows) {

                    updateFrameBuffer(light.shadowMap, shadowMapSpec, glm::vec2(env->renderSettings->shadowMapResolution, env->renderSettings->shadowMapResolution));

                    float near = 1.0f;
                    float far = 100.0f;
                    float size = 32.0f;

                    Transform directionTransform;
                    directionTransform.rotation = transform.rotation;
                    glm::vec3 direction = directionTransform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 1);

                    glm::mat4 projection = glm::ortho(-size, size, -size, size, near, far);
                    glm::mat4 view = glm::lookAt(transform.position - direction, transform.position, { 0, 1, 0 });
                    glm::mat4 viewProjection = projection * view;

                    frustum.viewProjectionMatrix = viewProjection;

                    shadowEnvData.projection = projection;
                    shadowEnvData.viewProjection = viewProjection;
                    shadowEnvData.view = view;
                    shadowEnvData.eyePosition = transform.position;
                    shadowEnvData.lightCount = 0;
                    shadowEnvData.radianceMapIndex = -1;
                    shadowEnvData.irradianceMapIndex = -1;
                    env->renderPipeline->addCallbackStep([&]() {
                        shadowEnvBuffer->setData(&shadowEnvData, sizeof(shadowEnvData));
                    });

                    env->world->each<const Transform, const MeshComponent>([&](EntityId id, const Transform& t, const MeshComponent& m) {
                        Mesh* mesh = m.mesh.get();
                        if (!mesh) {
                            mesh = quadMesh.get();
                        }

                        if (env->renderSettings->enableFrustumCulling && !frustum.inFrustum(t.getMatrix(), mesh)) {
                            return;
                        }
                        auto* batch = shadowBatches.get(shadowShader.get(), mesh);
                        if (batch->isInitialized()) {
                            batch->add(t.getMatrix(), nullptr, color::white, -1);
                        }
                    });

                    for (auto& i : shadowBatches.batches) {
                        for (auto& j : i.second) {
                            if (j.second->isInitialized()) {
                                j.second->environmentBuffer = shadowEnvBuffer;
                                j.second->defaultTexture = defaultTexture.get();
                                j.second->submit(light.shadowMap.get(), RenderPipeline::SHADOWS);
                                j.second->reset();
                            }
                        }
                    }

                }
            });
        }
    }

    void Renderer::submitPostProcessing() {
        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::POST_PROCESSING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_OFF, RenderPipeline::POST_PROCESSING);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::BLEND_OFF, RenderPipeline::POST_PROCESSING);

        auto dc = env->renderPipeline->addDrawCallStep(RenderPipeline::POST_PROCESSING, false);
        dc->name = "color grading";

        dc->vertexArray = &quadMesh->vertexArray;
        dc->shader = colorGradingShader.get();
        dc->frameBuffer = postProcessingBuffer.get();
        dc->textures.push_back(lightAccumulationBuffer->getAttachment(TextureAttachment::COLOR).get());

        Transform quadTransform;
        quadTransform.rotation.x = glm::radians(90.0f);
        quadTransform.scale = { 2, 2, -2 };

        dc->shaderState = Ref<ShaderState>::make();
        dc->shaderState->set("uTransform", quadTransform.calculateLocalMatrix());
        dc->shaderState->set("hueShift", env->renderSettings->hueShift);
        dc->shaderState->set("saturation", env->renderSettings->saturation);
        dc->shaderState->set("temperature", env->renderSettings->temperature);
        dc->shaderState->set("contrast", env->renderSettings->contrast);
        dc->shaderState->set("brightness", env->renderSettings->brightness);
        dc->shaderState->set("gamma", env->renderSettings->gamma);
        dc->shaderState->set("gain", env->renderSettings->gain);

        std::vector<int> textureSlots;
        for (int i = 0; i < dc->textures.size(); i++) {
            textureSlots.push_back(i);
        }
        dc->shaderState->set("uTextures", textureSlots.data(), textureSlots.size());
    }

}
