//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshRenderer.h"
#include "MeshFactory.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tridot {

    class Instance {
    public:
        glm::mat4 transform;
        Color color;
        float textureUnit;
        glm::vec2 texCoordsTopLeft;
        glm::vec2 texCoordsBottomRight;
    };

    MeshRenderer::MeshRenderer() {
        quadMesh = nullptr;
        blankTexture = nullptr;
        defaultShader = nullptr;
        frameBuffer = nullptr;
        maxBatchSize = 10000;
    }

    void MeshRenderer::init(uint32_t maxBatchSize) {
        this->maxBatchSize = maxBatchSize;
        quadMesh = MeshFactory::createQuad();

        Image image;
        Color color = Color::white;
        image.init(1, 1, 4, 8, &color, 1);
        blankTexture = Ref<Texture>::make();
        blankTexture->load(image);

        this->defaultShader = Ref<Shader>::make();
        this->defaultShader->load("../res/shaders/mesh.glsl");
    }

    void MeshRenderer::begin(glm::mat4 projection, Ref<FrameBuffer> frameBuffer) {
        this->projection = projection;
        this->frameBuffer = frameBuffer;
    }

    void MeshRenderer::submit(const MeshRenderer::SubmitCall &call, Texture *texture, Mesh *mesh, Shader *shader) {
        if(texture == nullptr){
            texture = blankTexture.get();
        }
        if(mesh == nullptr){
            mesh = quadMesh.get();
        }
        if(shader == nullptr){
            shader = defaultShader.get();
        }

        Batch *batch = nullptr;
        for(auto &b : batches){
            if(b){
                if(b->mesh == mesh && b->shader == shader){
                    batch = b.get();
                }
            }
        }

        if(batch == nullptr){
            Ref<Batch> b(true);
            b->init(sizeof(Instance), maxBatchSize, 32, mesh, shader,{
                {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4},
                {UINT8, 4, true},
                {FLOAT, 1},
                {FLOAT,  2},
                {FLOAT, 2}});
            batches.push_back(b);
            batch = b.get();
        }

        uint32_t unit = batch->getTextureUnit(texture);
        if(batch->instanceIndex >= batch->maxInstanceCount || unit == -1){
            flushBatch(batch);
            unit = batch->getTextureUnit(texture);
        }

        glm::mat4 transform(1);
        transform = glm::translate(transform, call.position);
        transform = glm::scale(transform, call.scale);
        if(call.rotation.x != 0){
            transform = glm::rotate(transform, call.rotation.x, {1, 0, 0});
        }
        if(call.rotation.y != 0){
            transform = glm::rotate(transform, call.rotation.y, {0, 1, 0});
        }
        if(call.rotation.z != 0){
            transform = glm::rotate(transform, call.rotation.z, {0, 0, 1});
        }

        Instance *i = (Instance*)batch->next();
        i->transform = transform;
        i->color = call.color;
        i->textureUnit = (float)unit;
        i->texCoordsTopLeft = call.texCoordsTopLeft;
        i->texCoordsBottomRight = call.texCoordsBottomRight;
    }

    void MeshRenderer::end() {
        for(auto &batch : batches){
            if(batch){
                flushBatch(batch.get());
            }
        }
    }

    void MeshRenderer::flushBatch(Batch *batch) {
        if(batch->instanceIndex > 0) {
            batch->shader->bind();
            if (frameBuffer) {
                frameBuffer->bind();
            } else {
                FrameBuffer::unbind();
            }

            batch->shader->set("uProjection", projection);
            int textures[32];
            for (int i = 0; i < 32; i++) {
                textures[i] = i;
            }
            batch->shader->set("uTextures", textures, 32);

            batch->updateBuffer();
            batch->submit();
            batch->resetTextures();
            batch->reset();
        }
    }

}