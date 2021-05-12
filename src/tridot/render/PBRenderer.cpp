//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "PBRenderer.h"
#include "MeshFactory.h"
#include "BatchBuffer.h"
#include "tridot/engine/Profiler.h"
#include <GL/gl.h>

namespace tridot {

    class PBInstance {
    public:
        glm::mat4 transform;
        float materialIndex;
        Color color;
        int id;
    };

    class PBLight{
    public:
        glm::vec3 position;
        int align1;
        glm::vec3 color;
        int align2;
        float intensity;
        int type;
        int align3;
        int align4;
    };

    class PBMaterial{
    public:

        glm::vec4 color;
        int mapping;
        float roughness;
        float metallic;
        float normalMapFactor;

        int texture;
        int normalMap;
        int roughnessMap;
        int metallicMap;

        glm::vec2 textureOffset;
        glm::vec2 textureScale;

        glm::vec2 normalMapOffset;
        glm::vec2 normalMapScale;

        glm::vec2 roughnessMapOffset;
        glm::vec2 roughnessMapScale;

        glm::vec2 metallicMapOffset;
        glm::vec2 metallicMapScale;
    };

    class PBRenderer::PBBatch{
    public:
        BatchBuffer instances;
        BatchBuffer materials;
        std::unordered_map<Texture*, int> textureMap;
        std::unordered_map<Material*, int> materialMap;
        Ref<VertexArray> vertexArray;
        Shader *shader;
        Mesh *mesh;
        uint32_t meshId;
    };

    PBRenderer::PBRenderer() {
        defaultMesh = nullptr;
        blankTexture = nullptr;
        defaultShader = nullptr;
        defaultMaterial = nullptr;
        frameBuffer = nullptr;
    }

    template<typename T>
    int mapIndex(std::unordered_map<T, int> &map, T t, uint32_t maxIndexCount){
        auto entry = map.find(t);
        if(entry == map.end()){
            if(maxIndexCount != -1 && map.size() >= maxIndexCount){
                return -1;
            }
            map[t] = map.size();
            return map[t];
        }else{
            return entry->second;
        }
    }

    void PBRenderer::init(const Ref<Shader> &shader, uint32_t maxTextures, uint32_t maxMaterials, uint32_t maxInstances) {
        this->maxTextures = maxTextures;
        this->maxMaterials = maxMaterials;
        this->maxInstances = maxInstances;

        defaultMesh = MeshFactory::createQuad();

        Image image;
        Color color = Color::white;
        image.init(1, 1, 4, 8, &color, 1);
        blankTexture = Ref<Texture>::make();
        blankTexture->load(image);

        defaultShader = shader;
        defaultMaterial = Ref<Material>::make();

        lights.init(sizeof(PBLight), UNIFORM_BUFFER);
    }

    void PBRenderer::begin(const glm::mat4 &projection, const glm::vec3 &cameraPosition, const Ref<FrameBuffer> &frameBuffer) {
        this->projection = projection;
        this->cameraPosition = cameraPosition;
        this->frameBuffer = frameBuffer;
    }

    void PBRenderer::submit(const Light &light, const glm::vec3 &positionOrDirection) {
        PBLight *l = (PBLight*)lights.next();
        l->type = (int)light.type;
        l->position = positionOrDirection;
        l->color = light.color;
        l->intensity = light.intensity;
    }

    void PBRenderer::submit(const glm::mat4 &transform, Color color, Mesh *mesh, Material *material, int id) {
        //setup defaults
        if(mesh == nullptr){
            mesh = defaultMesh.get();
        }
        if(material == nullptr){
            material = defaultMaterial.get();
        }
        Shader *shader = material->shader.get();
        if(shader == nullptr){
            shader = defaultShader.get();
        }

        //all resources need to be loaded
        if(mesh->vertexArray.getId() == 0){
            return;
        }
        if(shader->getId() == 0){
            return;
        }

        //get batch
        PBBatch *batch = nullptr;
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

        //init batch
        if(batch == nullptr || mesh->vertexArray.getId() != batch->meshId){
            Ref<PBBatch> b = Ref<PBBatch>::make();

            b->instances.init(sizeof(PBInstance), VERTEX_BUFFER);
            b->materials.init(sizeof(PBMaterial), UNIFORM_BUFFER);
            b->mesh = mesh;
            b->meshId = mesh->vertexArray.getId();
            b->shader = shader;

            b->vertexArray = Ref<VertexArray>::make(mesh->vertexArray);
            b->vertexArray->addVertexBuffer(b->instances.buffer, {
                {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4},//transform
                {FLOAT, 1},//materialIndex
                {UINT8, 4, true},//color
                {UINT8, 4, true},//id
            }, 1);

            batches.push_back(b);
            batch = b.get();
        }


        auto getMaterialIndex = [&](Material *material) -> uint32_t{
            //get material
            uint32_t materialIndex = mapIndex(batch->materialMap, material, maxMaterials);

            //init material
            bool textureLimitReached = false;
            if(materialIndex >= batch->materials.size()){
                PBMaterial *m = (PBMaterial*)batch->materials.next();
                m->color = material->color.vec();
                m->mapping = (int)material->mapping;
                m->roughness = material->roughness;
                m->metallic = material->metallic;
                m->normalMapFactor = material->normalMapFactor;

                m->textureScale = material->textureScale;
                m->textureOffset = material->textureOffset;

                m->normalMapScale = material->normalMapScale;
                m->normalMapOffset = material->normalMapOffset;

                m->roughnessMapScale = material->roughnessMapScale;
                m->roughnessMapOffset = material->roughnessMapOffset;

                m->metallicMapScale = material->metallicMapScale;
                m->metallicMapOffset = material->metallicMapOffset;

                if(material->texture.get() != nullptr){
                    m->texture = mapIndex(batch->textureMap, material->texture.get(), maxTextures);
                    textureLimitReached |= (m->texture == -1);
                }else{
                    m->texture = -1;
                }

                if(material->normalMap.get() != nullptr){
                    m->normalMap = mapIndex(batch->textureMap, material->normalMap.get(), maxTextures);
                    textureLimitReached |= (m->normalMap == -1);
                }else{
                    m->normalMap = -1;
                }

                if(material->roughnessMap.get() != nullptr){
                    m->roughnessMap = mapIndex(batch->textureMap, material->roughnessMap.get(), maxTextures);
                    textureLimitReached |= (m->roughnessMap == -1);
                }else{
                    m->roughnessMap = -1;
                }

                if(material->metallicMap.get() != nullptr){
                    m->metallicMap = mapIndex(batch->textureMap, material->metallicMap.get(), maxTextures);
                    textureLimitReached |= (m->metallicMap == -1);
                }else{
                    m->metallicMap = -1;
                }
            }

            if(textureLimitReached){
                materialIndex = -1;
            }
            return materialIndex;
        };

        //material index
        uint32_t materialIndex = getMaterialIndex(material);
        if(batch->instances.size() >= maxInstances || materialIndex == -1){
            flushBatch(batch);
            materialIndex = getMaterialIndex(material);
        }

        PBInstance *i = (PBInstance*)batch->instances.next();
        i->transform = transform;
        i->materialIndex = (float)materialIndex;
        i->color = color;
        i->id = id;
    }

    void PBRenderer::end() {
        for(auto &batch : batches){
            if(batch){
                flushBatch(batch.get());
            }
        }
        lights.reset();
        glFlush();
    }

    void PBRenderer::flushBatch(PBBatch *batch) {
        if(batch->instances.size() > 0) {
            batch->shader->bind();

            if (frameBuffer) {
                frameBuffer->bind();
            } else {
                FrameBuffer::unbind();
            }

            batch->shader->set("uProjection", projection);
            batch->shader->set("uCameraPosition", cameraPosition);

            int textures[32];
            for (int i = 0; i < 32; i++) {
                textures[i] = i;
            }
            batch->shader->set("uTextures", textures, 32);

            //instances
            batch->instances.update();

            //lights
            lights.update();
            batch->shader->set("uLights", lights.buffer.get());
            batch->shader->set("uLightCount", (int)lights.size());

            //materials
            batch->materials.update();
            batch->shader->set("uMaterials", batch->materials.buffer.get());


            //textures
            for(auto &texture : batch->textureMap){
                texture.first->bind(texture.second);
            }

            //draw call
            {
                TRI_PROFILE("render/draw call")
                batch->vertexArray->submit(-1, batch->instances.size());
            }

            batch->textureMap.clear();
            batch->materialMap.clear();
            batch->materials.reset();
            batch->instances.reset();
        }
    }

}