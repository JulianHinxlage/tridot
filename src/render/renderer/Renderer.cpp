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
        env->jobManager->addJob("Renderer", {"Renderer"});
    }

    void generateCubeSphere(Ref<Mesh> &mesh, int faceSections = 2) {
        glm::vec3 faceCenters[6] = {
            {1, 0, 0},
            {-1, 0, 0},
            {0, 1, 0},
            {0, -1, 0},
            {0, 0, 1},
            {0, 0, -1},
        };
        glm::vec3 faceX[6] = {
            {0, 1, 0},
            {0, 1, 0},
            {0, 0, 1},
            {0, 0, 1},
            {1, 0, 0},
            {1, 0, 0},
        };
        glm::vec3 faceY[6] = {
            {0, 0, 1},
            {0, 0, -1},
            {1, 0, 0},
            {-1, 0, 0},
            {0, 1, 0},
            {0, -1, 0},
        };

        std::vector<float> vs;

        auto add = [&](glm::vec3 v) {
            vs.push_back(v.x);
            vs.push_back(v.y);
            vs.push_back(v.z);
            for (int i = 0; i < 5; i++) {
                vs.push_back(0);
            }
        };

        for (int face = 0; face < 6; face++) {
            glm::vec3 c = faceCenters[face];
            glm::vec3 x = faceX[face];
            glm::vec3 y = faceY[face];

            for (int i = 0; i < faceSections; i++) {
                for (int j = 0; j < faceSections; j++) {
                    glm::vec3 v0 = c * 0.5f - x * 0.5f - y * 0.5f;
                    v0 += x / (float)faceSections * (float)i;
                    v0 += y / (float)faceSections * (float)j;

                    glm::vec3 v1 = v0 + x / (float)faceSections;
                    glm::vec3 v2 = v0 + x / (float)faceSections + y / (float)faceSections;
                    glm::vec3 v3 = v0 + y / (float)faceSections;
    
                    add(v0);
                    add(v1);
                    add(v3);

                    add(v1);
                    add(v2);
                    add(v3);
                }
            }
        }

        std::vector<int> is;
        for (int i = 0; i < vs.size(); i += 8) {
           glm::vec3 v = { vs[i + 0], vs[i + 1], vs[i + 2] };
           v = glm::normalize(v);

           vs[i + 0] = v.x;
           vs[i + 1] = v.y;
           vs[i + 2] = v.z;

           vs[i + 3] = v.x;
           vs[i + 4] = v.y;
           vs[i + 5] = v.z;

           is.push_back(i / 8);
        }

        mesh = Ref<Mesh>::make();
        mesh->create(vs.data(), vs.size(), is.data(), is.size(), {{FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2}});
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
            quadMesh = Ref<Mesh>::make();
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
            quadMesh->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });

            envBuffer = Ref<Buffer>::make();
            envBuffer->init(nullptr, 0, sizeof(envData), BufferType::UNIFORM_BUFFER, true);

            geometryShader = env->assetManager->get<Shader>("shaders/geometry.glsl");
            ambientLightShader = env->assetManager->get<Shader>("shaders/ambientLight.glsl");
            directionalLightShader = env->assetManager->get<Shader>("shaders/directionalLight.glsl");
            pointLightShader = env->assetManager->get<Shader>("shaders/pointLight.glsl");
            spotLightShader = env->assetManager->get<Shader>("shaders/spotLight.glsl");
            coneMesh = env->assetManager->get<Mesh>("models/cone.obj");
            
            generateCubeSphere(sphereMesh, 4);
            
            pointLightBatch.instanceBuffer = Ref<BatchBuffer>::make();
            pointLightBatch.instanceBuffer->init(sizeof(LightBatch::Instance));
        
            spotLightBatch.instanceBuffer = Ref<BatchBuffer>::make();
            spotLightBatch.instanceBuffer->init(sizeof(LightBatch::Instance));
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
        env->renderPipeline->freeOnThread(envBuffer);
        env->renderPipeline->freeOnThread(gBuffer);
        env->renderPipeline->freeOnThread(lightAccumulationBuffer);
        env->renderPipeline->freeOnThread(transparencyBuffer);
        env->renderPipeline->freeOnThread(sphereMesh);
        env->renderPipeline->freeOnThread(coneMesh);
        env->renderPipeline->freeOnThread(pointLightBatch.vertexArray);
        env->renderPipeline->freeOnThread(pointLightBatch.instanceBuffer);
        env->renderPipeline->freeOnThread(spotLightBatch.vertexArray);
        env->renderPipeline->freeOnThread(spotLightBatch.instanceBuffer);

        defaultTexture = nullptr;
        defaultMaterial = nullptr;
        quadMesh = nullptr;
        geometryShader = nullptr;
        ambientLightShader = nullptr;
        directionalLightShader = nullptr;
        pointLightShader = nullptr;
        spotLightShader = nullptr;
        envBuffer = nullptr;
        gBuffer = nullptr;
        lightAccumulationBuffer = nullptr;
        transparencyBuffer = nullptr;
        sphereMesh = nullptr;
        coneMesh = nullptr;
        pointLightBatch.vertexArray = nullptr;
        pointLightBatch.instanceBuffer = nullptr;
        spotLightBatch.vertexArray = nullptr;
        spotLightBatch.instanceBuffer = nullptr;

        drawList.reset();
        batches.clear();
        transparencyBatches.clear();
    }

    void Renderer::tick() {
        updateFrameBuffer(gBuffer, gBufferSpec);
        updateFrameBuffer(lightAccumulationBuffer, lightAccumulationSpec);


        prepareTransparencyBuffer();
        prepareLightBatches();


        env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::GEOMETRY);
        env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_BACK, RenderPipeline::GEOMETRY);

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
                submitMeshes();

                env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_ON, RenderPipeline::TRANSPARENCY);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_BACK, RenderPipeline::TRANSPARENCY);


                submitBatches(camera);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::DEPTH_OFF, RenderPipeline::LIGHTING);
                env->renderPipeline->addCommandStep(RenderPipeline::Command::CULL_FRONT, RenderPipeline::LIGHTING);
                
                //addetive blending with no alpha
                env->renderPipeline->addCallbackStep([]() {
                    glEnable(GL_BLEND);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFunc(GL_ONE, GL_ONE);
                }, RenderPipeline::LIGHTING);

                submitLights(camera);
                submitPointLightBatch();
                submitSpotLightBatch();

                camera.output = lightAccumulationBuffer;
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
    }

    void Renderer::submit(const glm::mat4& transform, Mesh* mesh, Material* material, Color color, EntityId id) {
        if (!mesh) {
            mesh = quadMesh.get();
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

    bool Renderer::updateFrameBuffer(Ref<FrameBuffer>& frameBuffer, const std::vector<FrameBufferAttachmentSpec>& spec) {
        TRI_PROFILE_FUNC();
        if (frameBuffer == nullptr) {
            env->renderPipeline->freeOnThread(frameBuffer);
            env->renderPipeline->addCallbackStep([&]() {
                frameBuffer = Ref<FrameBuffer>::make();
                frameBuffer->init(env->viewport->size.x, env->viewport->size.y, spec);
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

    void Renderer::submitBatches(Camera &c) {
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
        light.textureFormat = TextureFormat::RGBA8;

        lightAccumulationSpec = { light };
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
        switch (light.type) {
        case Light::AMBIENT_LIGHT: {
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
        case Light::DIRECTIONAL_LIGHT: {
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
            return true;
        }
        case Light::POINT_LIGHT: {
            LightBatch::Instance* iData = (LightBatch::Instance*)pointLightBatch.instanceBuffer->next();

            Transform positionTransform;
            positionTransform.decompose(transform.getMatrix());

            Transform sphereTransform;
            sphereTransform.position = positionTransform.position;
            sphereTransform.scale = glm::vec3(2, 2, 2) * light.range;
            glm::mat sphereMatrix = camera.viewProjection * sphereTransform.calculateLocalMatrix();

            Transform directionTransform;
            directionTransform.rotation = transform.rotation;
            glm::vec3 direction = directionTransform.calculateLocalMatrix() * glm::vec4(1, 0, 0, 1);

            iData->transform = sphereMatrix;
            iData->position = positionTransform.position;
            iData->direction = direction;
            iData->color = light.color;
            iData->intensity = light.intensity;
            iData->range = light.range;
            iData->falloff = light.falloff;
            iData->spotAngle = glm::radians(light.spotAngle);
            return true;
        }
        case Light::SPOT_LIGHT: {
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
            glm::mat coneMatrix = camera.viewProjection * (coneTransform.calculateLocalMatrix() * offset.calculateLocalMatrix());

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
        default:
            return false;
        }
    }

}