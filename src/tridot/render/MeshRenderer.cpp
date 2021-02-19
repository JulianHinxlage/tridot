//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshRenderer.h"
#include "MeshFactory.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

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

    void MeshRenderer::init(Ref<Shader> shader, uint32_t maxBatchSize) {
        this->maxBatchSize = maxBatchSize;
        quadMesh = MeshFactory::createQuad();

        Image image;
        Color color = Color::white;
        image.init(1, 1, 4, 8, &color, 1);
        blankTexture = Ref<Texture>::make();
        blankTexture->load(image);

        this->defaultShader = shader;
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

        if(mesh->vertexArray.getId() == 0){
            return;
        }
        if(shader->getId() == 0){
            return;
        }
        if(texture->getId() == 0){
            return;
        }

        Batch *batch = nullptr;
        for(int i = 0; i < batches.size(); i++){
            auto &b = batches[i];
            if(b){
                if(b->mesh == mesh && b->shader == shader){
                    batch = b.get();
                    if(batch){
                        if(mesh->vertexArray.getId() != batch->meshId){
                            batches.erase(batches.begin() + i);
                            i--;
                            batch = nullptr;
                            continue;
                        }
                    }
                    break;
                }
            }
        }


        if(batch == nullptr || mesh->vertexArray.getId() != batch->meshId){
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
        if(call.rotation != glm::vec3(0, 0, 0)){
            transform = transform * glm::eulerAngleXYZ(call.rotation.x, call.rotation.y, call.rotation.z);
        }
        transform = glm::scale(transform, call.scale);

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