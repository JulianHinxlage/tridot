//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "MeshRendererBatch.h"

namespace tridot {

    MeshRendererBatch::MeshRendererBatch() {
        shader = nullptr;
        mesh = nullptr;
        elementSize = 0;
        instanceIndex = 0;
        updateIndex = 0;
        maxTextureCount = 0;
        maxInstanceCount = 0;
    }

    void MeshRendererBatch::init(uint32_t elementSize, uint32_t maxInstanceCount, uint32_t maxTextureCount, Mesh *mesh, Shader *shader, std::vector<Attribute> layout) {
        this->elementSize = elementSize;
        this->maxTextureCount = maxTextureCount;
        this->maxInstanceCount = maxInstanceCount;
        this->mesh = mesh;
        this->meshId = mesh->vertexArray.getId();
        this->shader = shader;

        instanceBuffer = Ref<Buffer>::make();
        instanceBuffer->init(nullptr, elementSize * maxInstanceCount, elementSize, VERTEX_BUFFER, true);
        data.resize(elementSize * maxInstanceCount);

        vertexArray = Ref<VertexArray>::make(mesh->vertexArray);
        vertexArray->addVertexBuffer(instanceBuffer, layout, 1);
    }

    void *MeshRendererBatch::next() {
        return data.data() + instanceIndex++ * elementSize;
    }

    void MeshRendererBatch::updateBuffer() {
        if(instanceIndex > updateIndex){
            instanceBuffer->bind();
            instanceBuffer->setData(data.data() + updateIndex * elementSize, (instanceIndex - updateIndex) * elementSize, updateIndex * elementSize);
            updateIndex = instanceIndex;
        }
    }

    void MeshRendererBatch::submit() {
        for(auto &t : textures){
            t.first->bind(t.second);
        }
        vertexArray->submit(-1, instanceIndex);
    }

    void MeshRendererBatch::reset() {
        updateIndex = 0;
        instanceIndex = 0;
    }

    uint32_t MeshRendererBatch::getTextureUnit(Texture *texture) {
        auto entry = textures.find(texture);
        if(entry == textures.end()){
            uint32_t unit = textures.size();
            if(unit >= maxTextureCount){
                return -1;
            }
            textures[texture] = unit;
            return unit;
        }else{
            return entry->second;
        }
    }

    void MeshRendererBatch::resetTextures() {
        textures.clear();
    }

}