//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Renderer.h"
#include "core/core.h"
#include "RenderPipeline.h"
#include "ShaderState.h"
#include "engine/AssetManager.h"

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Renderer, env->renderer);

    //map asstes to an index and keep a list off the assets
    template<typename T>
    class AssetList {
    public:
        std::vector<T*> assets;
        std::unordered_map<T*, int> indexMap;

        int getIndex(T *asset) {
            if (asset == nullptr) {
                return -1;
            }
            auto x = indexMap.find(asset);
            if (x != indexMap.end()) {
                return x->second;
            }
            assets.push_back(asset);
            indexMap[asset] = assets.size() - 1;
            return assets.size() - 1;
        }

        void reset() {
            indexMap.clear();
            assets.clear();
        }
    };

    //insatnce data for shader
    struct InstanceData {
    public:
        glm::mat4 transform;
        Color color;
        float materialIndex;
        uint32_t id;
    };

    //material data for shader
    class MaterialData {
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
        int ambientOcclusionMap;
        int displacementMap;
        int align1;
        int align2;

        glm::vec2 textureOffset;
        glm::vec2 textureScale;

        glm::vec2 normalMapOffset;
        glm::vec2 normalMapScale;

        glm::vec2 roughnessMapOffset;
        glm::vec2 roughnessMapScale;

        glm::vec2 metallicMapOffset;
        glm::vec2 metallicMapScale;

        glm::vec2 ambientOcclusionMapOffset;
        glm::vec2 ambientOcclusionMapScale;

        glm::vec2 displacementMapOffset;
        glm::vec2 displacementMapScale;
    };

    //light data for shader
    class LightData {
    public:
        glm::vec3 position;
        int align1;
        glm::vec3 direction;
        int align2;
        glm::vec3 color;
        int align3;
        int type;
        float intensity;
        int shadowMapIndex;
        int align4;
        glm::mat4 projection;
    };

    //environment data for shader
    class EnvironmentData {
    public:
        glm::mat4 projection;
        glm::vec3 cameraPosition;
        int align1;
        int lightCount;
        float environmentMapIntensity;
        int radianceMapIndex;
        int irradianceMapIndex;
    };

    class Renderer::Impl {
    public:
        class Batch {
        public:
            Mesh* mesh = nullptr;
            Shader* shader = nullptr;
            Ref<VertexArray> vertexArray;
            Ref<BatchBuffer> instances;
            AssetList<Texture> textures;
            AssetList<Material> materials;
            Ref<BatchBuffer> materialBuffer;
        };

        std::vector<std::vector<Ref<Batch>>> batches;
        Ref<BatchBuffer> lightBuffer;
        Ref<BatchBuffer> environmentBuffer;
        EnvironmentData environment;
        Ref<Texture> radianceMap;
        Ref<Texture> irradianceMap;

        void startup() {
            lightBuffer = Ref<BatchBuffer>::make();
            lightBuffer->init(sizeof(LightData), UNIFORM_BUFFER);

            environmentBuffer = Ref<BatchBuffer>::make();
            environmentBuffer->init(sizeof(EnvironmentData), UNIFORM_BUFFER);
        }

        Batch *getBatch(Mesh* mesh, Shader* shader) {
            if (batches.size() > shader->getId()) {
                auto& list = batches[shader->getId()];
                if (list.size() > mesh->vertexArray.getId()) {
                    Batch *batch = list[mesh->vertexArray.getId()].get();
                    if (batch) {
                        return batch;
                    }
                }
                else {
                    list.resize(mesh->vertexArray.getId() + 1);
                }
            }
            else {
                batches.resize(shader->getId() + 1);
                auto& list = batches[shader->getId()];
                list.resize(mesh->vertexArray.getId() + 1);
            }

            //create batch
            Ref<Batch> batch = Ref<Batch>::make();
            batches[shader->getId()][mesh->vertexArray.getId()] = batch;
            batch->mesh = mesh;
            batch->shader = shader;
            
            batch->instances = Ref<BatchBuffer>::make();
            batch->instances->init(sizeof(InstanceData));

            batch->materialBuffer= Ref<BatchBuffer>::make();
            batch->materialBuffer->init(sizeof(MaterialData), UNIFORM_BUFFER);

            batch->vertexArray = Ref<VertexArray>::make(batch->mesh->vertexArray);
            batch->vertexArray->addVertexBuffer(batch->instances->buffer, {
                {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                {UINT8, 4, true}, //color
                {FLOAT, 1}, //material index
                {UINT8, 4, true}, //id
            }, 1);
            return batch.get();
        }
    };

    void Renderer::startup() {
        renderPass = env->pipeline->getOrAddRenderPass("geometry");
        
        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, Color::white);
        defaultTexture = Ref<Texture>::make();
        defaultTexture->load(image);

        defaultShader = env->assets->get<Shader>("shaders/pbr.glsl");
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
    

        impl = Ref<Impl>::make();
        impl->startup();
    }

    void Renderer::beginScene(glm::mat4& projection, glm::vec3 position) {
        impl->environment.projection = projection;
        impl->environment.cameraPosition = position;
    }

    void Renderer::submit(const glm::vec3& position, const glm::vec3 direction, Light& light) {
        LightData* l = (LightData*)impl->lightBuffer->next();
        l->color = light.color.vec();
        l->direction = direction;
        l->position = position;
        l->intensity = light.intensity;
        l->projection = glm::mat4(1);
        l->shadowMapIndex = -1;
        l->type = (int)light.type;
    }
    
    void Renderer::setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity) {
        impl->environment.environmentMapIntensity = intensity;
        impl->radianceMap = radianceMap;
        impl->irradianceMap = irradianceMap;
    }
    
    void Renderer::submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id) {
        if (!mesh) {
            mesh = defaultMesh.get();
        }
        if (!material) {
            material = defaultMaterial.get();
        }
        Shader* shader = material->shader.get();
        if (!shader) {
            shader = defaultShader.get();
        }
        if (mesh->vertexArray.getId() != 0) {
            if (shader->getId() != 0) {
                Impl::Batch *batch = impl->getBatch(mesh, shader);
                InstanceData *i = (InstanceData*)batch->instances->next();
                i->transform = transform;
                i->materialIndex = batch->materials.getIndex(material);
                i->color = color;
                i->id = id;
            }
        }   
    }

    void Renderer::drawScene(Ref<FrameBuffer> frameBuffer) {
        this->frameBuffer = frameBuffer;
    }

    void Renderer::update() {
        renderPass->addCommand(DEPTH_ON).name = "depth on";

        //set environment
        EnvironmentData* e = (EnvironmentData*)impl->environmentBuffer->next();
        *e = impl->environment;
        e->lightCount = impl->lightBuffer->size();

        if (impl->radianceMap) {
            e->radianceMapIndex = 0;
            renderPass->addCallback([radianceMap = impl->radianceMap]() {
                radianceMap->bind(30);
            }).name = "bind radiance map";
        }
        else {
            e->radianceMapIndex = -1;
        }
        if (impl->irradianceMap) {
            e->irradianceMapIndex = 1;
            renderPass->addCallback([irradianceMap = impl->irradianceMap]() {
                irradianceMap->bind(31);
            }).name = "bind irradiance map";
        }
        else {
            e->irradianceMapIndex = -1;
        }

        impl->environment.radianceMapIndex = -1;
        impl->environment.irradianceMapIndex = -1;
        impl->environment.environmentMapIntensity = 0;
        impl->radianceMap = nullptr;
        impl->irradianceMap = nullptr;

        renderPass->addCallback([&]() {
            impl->lightBuffer->update();
            impl->lightBuffer->reset();
            impl->environmentBuffer->update();
            impl->environmentBuffer->reset();
        }).name = "environment";

        //instance shader
        for (auto& list : impl->batches) {
            for (auto& batch : list) {
                if (batch) {
                    if (batch->instances->size() > 0) {

                        //set materials
                        for (auto& mat : batch->materials.assets) {
                            if (mat != nullptr) {
                                MaterialData* m = (MaterialData*)batch->materialBuffer->next();
                                m->texture = batch->textures.getIndex(mat->texture.get());
                                m->normalMap = batch->textures.getIndex(mat->normalMap.get());
                                m->roughnessMap = batch->textures.getIndex(mat->roughnessMap.get());
                                m->metallicMap = batch->textures.getIndex(mat->metallicMap.get());
                                m->ambientOcclusionMap = batch->textures.getIndex(mat->ambientOcclusionMap.get());

                                m->color = mat->color.vec();
                                m->mapping = (int)mat->mapping;
                                m->roughness = mat->roughness;
                                m->metallic = mat->metallic;
                                m->normalMapFactor = mat->normalMapFactor;

                                m->textureOffset = mat->textureOffset + mat->offset;
                                m->textureScale = mat->textureScale * mat->scale;
                                m->normalMapOffset = mat->normalMapOffset + mat->offset;
                                m->normalMapScale = mat->normalMapScale * mat->scale;
                                m->roughnessMapOffset = mat->roughnessMapOffset + mat->offset;
                                m->roughnessMapScale = mat->roughnessMapScale * mat->scale;
                                m->metallicMapOffset = mat->metallicMapOffset + mat->offset;
                                m->metallicMapScale = mat->metallicMapScale * mat->scale;
                                m->ambientOcclusionMapOffset = mat->ambientOcclusionMapOffset + mat->offset;
                                m->ambientOcclusionMapScale = mat->ambientOcclusionMapScale * mat->scale;
                            }
                        }
                        batch->materials.reset();

                        auto file = env->assets->getFile(batch->mesh);

                        //set instances
                        renderPass->addCallback([&]() {
                            batch->instances->update();
                            batch->instances->reset();

                            batch->materialBuffer->update();
                            batch->materialBuffer->reset();
                        }).name = "instances " + file;

                        //set draw call
                        auto& step = renderPass->addDrawCall("mesh " + file);
                        step.shader = batch->shader;
                        step.frameBuffer = frameBuffer;
                        step.mesh = batch->mesh;
                        step.vertexArray = batch->vertexArray.get();
                        step.insatnceCount = batch->instances->size();


                        //set textures
                        for (auto& tex : batch->textures.assets) {
                            if (tex != nullptr) {
                                step.textures.push_back(tex);
                            }
                        }
                        batch->textures.reset();

                        step.shaderState = Ref<ShaderState>::make();
                        step.shaderState->set("uMaterials", batch->materialBuffer->buffer.get());
                        step.shaderState->set("uLights", impl->lightBuffer->buffer.get());
                        step.shaderState->set("uEnvironment", impl->environmentBuffer->buffer.get());

                        int cubeTextures[2] = { 30, 31 };
                        step.shaderState->set("uCubeTextures", cubeTextures, 2);
                    }
                }
            }
        }
    }

    void Renderer::resetScene() {

    }

    void Renderer::shutdown() {

    }

}

/*
#include "pch.h"
#include "core/core.h"
#include "Renderer.h"
#include "engine/AssetManager.h"
#include "BatchBuffer.h"
#include "render/RenderContext.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Renderer, env->renderer);

    class Renderer::DrawList {
    public:
        glm::mat4 projectionMatrix;
        glm::vec3 eyePosition;
        Ref<Texture> radianceMap;
        Ref<Texture> irradianceMap;
        float environmentMapIntensity = 0;

        int drawCallCount = 0;
        int instanceCount = 0;
        int meshCount = 0;
        int materialCount = 0;
        int shaderCount = 0;
        int lightCount = 0;

        class InstanceData {
        public:
            glm::mat4 transform;
            Color color;
            float materialIndex;
            uint32_t id;
        };

        class MaterialData {
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
            int ambientOcclusionMap;
            int displacementMap;
            int align1;
            int align2;

            glm::vec2 textureOffset;
            glm::vec2 textureScale;

            glm::vec2 normalMapOffset;
            glm::vec2 normalMapScale;

            glm::vec2 roughnessMapOffset;
            glm::vec2 roughnessMapScale;

            glm::vec2 metallicMapOffset;
            glm::vec2 metallicMapScale;

            glm::vec2 ambientOcclusionMapOffset;
            glm::vec2 ambientOcclusionMapScale;

            glm::vec2 displacementMapOffset;
            glm::vec2 displacementMapScale;
        };

        class LightData {
        public:
            glm::vec3 position;
            int align1;
            glm::vec3 direction;
            int align2;
            glm::vec3 color;
            int align3;
            int type;
            float intensity;
            int shadowMapIndex;
            int align4;
            glm::mat4 projection;
        };

        class LightEntry {
        public:
            LightData data;
            Ref<FrameBuffer> shadowMap;
            glm::mat4 projection;
            LightType type;
        };

        class EnvironmentData{
        public:
            glm::mat4 projection;
            glm::vec3 cameraPosition;
            int align1;
            int lightCount;
            float environmentMapIntensity;
            int environmentMapIndex;
            int irradianceMapIndex;
        };

        class SortKey {
        public:
            uint64_t v1;

            bool operator<(const SortKey &key) {
                return v1 < key.v1;
            }
        };

        class Entry {
        public:
            SortKey key;
            uint32_t index;

            bool operator<(const Entry& entry) {
                return key < entry.key;
            }
        };

        std::vector<Entry> drawList;

        class Call {
        public:
            glm::mat4 transform;
            Color color;
            Material* material;
            Mesh *mesh;
            uint32_t id;
        };
        std::vector<Call> calls;

        std::vector<Shader*> shaderList;
        int nullShaderIndex = -1;
        std::vector<Mesh*> meshList;
        std::vector<Material*> materialList;
        std::vector<Texture*> textureList;
        std::vector<LightEntry> lightList;

        Ref<Shader> defaultShader;
        Ref<FrameBuffer> shadowMapFrameBuffer;

        class MeshBatch {
        public:
            Ref<BatchBuffer> instances;
            Ref<VertexArray> mesh;
            int meshId;
        };
        std::unordered_map<Mesh*, MeshBatch> meshes;
        BatchBuffer materialBuffer;
        BatchBuffer lightBuffer;
        Ref<Buffer> environmentBuffer;
        bool useMaterials = true;
        bool enableShadows = true;

        uint32_t getShaderIndex(Shader* shader) {
            if(!shader){
                if(nullShaderIndex == -1){
                    shaderList.push_back(shader);
                    nullShaderIndex = shaderList.size() - 1;
                }
                return nullShaderIndex;
            }
            if(shader->assetIndex == -1){
                shaderList.push_back(shader);
                shader->assetIndex = shaderList.size() - 1;
            }
            return shader->assetIndex;
        }

        uint32_t getMeshIndex(Mesh* mesh) {
            if(mesh->assetIndex == -1){
                meshList.push_back(mesh);
                mesh->assetIndex = meshList.size() - 1;
            }
            return mesh->assetIndex;
        }

        uint32_t getMaterialIndex(Material* material) {
            if(material->assetIndex == -1){
                materialList.push_back(material);
                material->assetIndex = materialList.size() - 1;
            }
            return material->assetIndex;
        }

        uint32_t getTextureIndex(Texture* texture) {
            if(!texture){
                return -1;
            }
            if(texture->assetIndex == -1){
                textureList.push_back(texture);
                texture->assetIndex = textureList.size() - 1;
            }
            return texture->assetIndex;
        }

        void resetTextures(){
            for(auto &texture : textureList){
                texture->assetIndex = -1;
            }
            textureList.clear();
        }
        void resetShaders(){
            for(auto &shader : shaderList){
                if(shader){
                    shader->assetIndex = -1;
                }
            }
            shaderList.clear();
            nullShaderIndex = -1;
        }
        void resetMaterials(){
            for(auto &material : materialList){
                material->assetIndex = -1;
            }
            materialList.clear();
        }
        void resetMeshes(){
            for(auto &mesh : meshList){
                mesh->assetIndex = -1;
            }
            meshList.clear();
        }

        void init() {
            materialBuffer.init(sizeof(MaterialData), UNIFORM_BUFFER);
            lightBuffer.init(sizeof(LightData), UNIFORM_BUFFER);
            environmentBuffer = Ref<Buffer>::make();
            environmentBuffer->init(nullptr, 0, sizeof(EnvironmentData), UNIFORM_BUFFER, true);

            env->console->setVariable("shadows", &enableShadows);
        }

        void addLight(const glm::vec3 &position, const glm::vec3 direction, Light &light){
            LightData *l = (LightData*)lightBuffer.next();
            l->color = light.color.vec();
            l->type = (int)light.type;
            l->position = position;
            l->direction = direction;
            l->intensity = light.intensity;

            if(light.shadowMap.get() == nullptr){
                light.shadowMap = Ref<FrameBuffer>::make();
                Ref<Texture> depth = Ref<Texture>::make();
                depth->create(2048, 2048, DEPTH32, false);
                depth->setMagMin(false, false);
                depth->setWrap(false, false);
                depth->setBorderColor(Color::white);
                light.shadowMap->setAttachment({DEPTH, Color(0)}, depth);
                light.shadowMap->resize(2048, 2048);
            }

            if(light.type == DIRECTIONAL_LIGHT){
                float near = 1.0f;
                float far = 100.0f;
                float size = 20.0f;
                glm::mat4 projection = glm::ortho(-size, size, -size, size, near, far);
                glm::mat4 view = glm::lookAt(position - direction, position, {0, 1, 0});
                l->projection = projection * view;
            }else{
                l->projection = glm::mat4(1);
            }
            lightList.push_back({*l, light.shadowMap, l->projection, light.type});
        }

        void add(const glm::mat4 &transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id, uint32_t layer = 0) {
            if(mesh->vertexArray.getId() == 0){
                return;
            }

            bool opaque = material->isOpaque() && color.a == 255;
            uint32_t depth = glm::length(eyePosition - position) / 0.0001;
            depth = std::min(depth, (uint32_t)(1 << 24) - 1);

            SortKey key;
            key.v1 = 0;
            key.v1 |= ((uint64_t)layer & 0x3f) << 58;
            key.v1 |= (uint64_t)!opaque << 57;
            if (opaque) {
                key.v1 |= ((uint64_t)getMeshIndex(mesh) & 0xffff) << 40;
                key.v1 |= ((uint64_t)getShaderIndex(material->shader.get()) & 0xffff) << 24;
                key.v1 |= ((uint64_t)depth & 0xffffff) << 0;
            }
            else {
                key.v1 |= ((uint64_t)-depth & 0xffffff) << 32;
                key.v1 |= ((uint64_t)getMeshIndex(mesh) & 0xffff) << 16;
                key.v1 |= ((uint64_t)getShaderIndex(material->shader.get()) & 0xffff) << 0;
            }
            getMaterialIndex(material);

            calls.push_back({transform, color, material, mesh, id});
            drawList.push_back({ key, (uint32_t)calls.size() - 1 });
        }

        void sort() {
            TRI_PROFILE("sort");
            std::sort(drawList.begin(), drawList.end());
        }

        BatchBuffer *prepareMesh(Mesh* mesh) {
            auto x = meshes.find(mesh);
            MeshBatch* batch = nullptr;
            if (x == meshes.end()) {
                batch = &meshes[mesh];
                batch->mesh = Ref<VertexArray>::make(mesh->vertexArray);
                batch->instances = Ref<BatchBuffer>::make();
                batch->instances->init(sizeof(InstanceData), VERTEX_BUFFER);
                batch->mesh->addVertexBuffer(batch->instances->buffer, {
                    {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, {FLOAT, 4}, //transform
                    {UINT8, 4, true}, //color
                    {FLOAT, 1}, //material index
                    {UINT8, 4, true} }, //id
                    1);
                batch->meshId = mesh->vertexArray.getId();
            }else{
                batch = &x->second;
                if(batch->meshId != mesh->vertexArray.getId()){
                    meshes.erase(mesh);
                    return prepareMesh(mesh);
                }
            }
            return batch->instances.get();
        }

        void submit(Mesh* mesh, Shader *shader) {
            MeshBatch& batch = meshes[mesh];
            if(batch.instances->size() > 0) {
                if(useMaterials){
                    materialBuffer.update();
                    shader->set("uMaterials", materialBuffer.buffer.get());
                }

                int textures[30];
                for (int i = 0; i < 30; i++) {
                    textures[i] = i;
                }
                for(int i = 0; i < textureList.size(); i++){
                    textureList[i]->bind(i);
                }

                shader->set("uTextures", textures, 30);

                batch.instances->update();
                batch.mesh->submit(-1, batch.instances->size());
                batch.instances->reset();
                RenderContext::flush(false);

                materialBuffer.reset();
                resetMaterials();
                resetTextures();
                drawCallCount++;
            }
        }

        void submitShadowMap(Mesh* mesh, Shader *shader) {
            MeshBatch& batch = meshes[mesh];
            if(batch.instances->size() > 0) {
                batch.instances->update();
                batch.mesh->submit(-1, batch.instances->size());
                batch.instances->reset();
                RenderContext::flush(false);
                drawCallCount++;
            }
        }

        void setMaterialData(Material *m, MaterialData *d){
            d->color = m->color.vec();
            d->mapping = (int)m->mapping;

            d->roughness = m->roughness;
            d->metallic = m->metallic;
            d->normalMapFactor = m->normalMapFactor;

            d->texture = getTextureIndex(m->texture.get());
            d->normalMap = getTextureIndex(m->normalMap.get());
            d->roughnessMap = getTextureIndex(m->roughnessMap.get());
            d->metallicMap = getTextureIndex(m->metallicMap.get());
            d->ambientOcclusionMap = getTextureIndex(m->ambientOcclusionMap.get());
            d->displacementMap = getTextureIndex(m->displacementMap.get());

            d->textureOffset = m->textureOffset + m->offset;
            d->textureScale = m->textureScale * m->scale;
            d->normalMapOffset = m->normalMapOffset + m->offset;
            d->normalMapScale = m->normalMapScale * m->scale;
            d->roughnessMapOffset = m->roughnessMapOffset + m->offset;
            d->roughnessMapScale = m->roughnessMapScale * m->scale;
            d->metallicMapOffset = m->metallicMapOffset + m->offset;
            d->metallicMapScale = m->metallicMapScale * m->scale;
            d->ambientOcclusionMapOffset = m->ambientOcclusionMapOffset + m->offset;
            d->ambientOcclusionMapScale = m->ambientOcclusionMapScale * m->scale;
            d->displacementMapOffset = m->displacementMapOffset + m->offset;
            d->displacementMapScale = m->displacementMapScale * m->scale;
        }

        void updateShadowMaps(){
            TRI_PROFILE("shadow maps");

            Mesh *mesh = nullptr;
            BatchBuffer *instances;
            Ref<Shader> shader = env->assets->get<Shader>("shaders/shadow.glsl");
            if(shader->getId() == 0){
                return;
            }
            shader->bind();

            for(auto &light : lightList) {
                if(light.type != DIRECTIONAL_LIGHT){
                    continue;
                }

                light.shadowMap->bind();
                light.shadowMap->clear();

                EnvironmentData e;
                e.projection = light.projection;
                e.cameraPosition = {0, 0, 0};
                e.lightCount = lightBuffer.size();
                e.environmentMapIndex = -1;
                e.irradianceMapIndex = -1;
                e.environmentMapIntensity = 0;
                environmentBuffer->setData(&e, sizeof(e));
                shader->set("uEnvironment", environmentBuffer.get());

                for (auto &i : drawList) {
                    Call &call = calls[i.index];

                    if (call.mesh != mesh) {
                        if (mesh) {
                            submitShadowMap(mesh, shader.get());
                        }
                        mesh = call.mesh;
                        instances = prepareMesh(mesh);
                    }

                    InstanceData *instance = (InstanceData *) instances->next();
                    instance->transform = call.transform;
                    instance->color = call.color;
                    instance->materialIndex = -1;
                    instance->id = call.id;
                    instanceCount++;
                }

                if (mesh) {
                    submitShadowMap(mesh, shader.get());
                }

                shadowMapFrameBuffer->unbind();
            }
        }

        void updateLightBuffer(Shader *shader){
            lightBuffer.reset();
            for(auto &light : lightList){
                if(enableShadows){
                    light.data.shadowMapIndex = getTextureIndex(light.shadowMap->getAttachment(DEPTH).get());
                    *(LightData*)lightBuffer.next() = light.data;
                }else{
                    light.data.shadowMapIndex = -1;
                    *(LightData*)lightBuffer.next() = light.data;
                }
            }
            lightBuffer.update();
            shader->set("uLights", lightBuffer.buffer.get());
        }

        void draw(Ref<FrameBuffer> frameBuffer) {
            TRI_PROFILE("draw");
            drawCallCount = 0;
            instanceCount = 0;

            if(enableShadows) {
                updateShadowMaps();
            }

            if (frameBuffer) {
                frameBuffer->bind();
            }
            else {
                FrameBuffer::unbind();
            }

            materialCount = materialList.size();
            lightCount = lightBuffer.size();
            shaderCount = shaderList.size();
            meshCount = meshList.size();

            Mesh* mesh = nullptr;
            Material* material = nullptr;
            BatchBuffer* instances;
            Shader *shader = nullptr;

            resetMaterials();
            int materialIndex = 0;
            useMaterials = true;

            {
                TRI_PROFILE("geometry");
                for (auto &i : drawList) {
                    Call &call = calls[i.index];

                    if (call.mesh != mesh) {
                        if (mesh) {
                            updateLightBuffer(shader);
                            submit(mesh, shader);
                            material = nullptr;
                        }
                        mesh = call.mesh;
                        instances = prepareMesh(mesh);
                    }

                    if (material != call.material) {
                        Shader *s = call.material->shader.get();
                        if (!s) {
                            s = defaultShader.get();
                        }
                        if (shader != s) {
                            if (shader) {
                                updateLightBuffer(shader);
                                submit(mesh, shader);
                                material = nullptr;
                            }

                            shader = s;
                            shader->bind();
                            useMaterials = shader->has("uMaterials");

                            EnvironmentData e;
                            e.projection = projectionMatrix;
                            e.cameraPosition = eyePosition;
                            e.lightCount = lightBuffer.size();
                            e.environmentMapIndex = -1;
                            e.irradianceMapIndex = -1;

                            if (radianceMap.get() && radianceMap->getId() != 0) {
                                e.environmentMapIndex = 0;
                                radianceMap->bind(30);
                            }
                            if (irradianceMap.get() && irradianceMap->getId() != 0) {
                                e.irradianceMapIndex = 1;
                                irradianceMap->bind(31);
                            }

                            int textures[2] = { 30, 31 };
                            for (int i = 0; i < textureList.size(); i++) {
                                textureList[i]->bind(i);
                            }
                            shader->set("uCubeTextures", textures, 2);

                            e.environmentMapIntensity = environmentMapIntensity;
                            environmentBuffer->setData(&e, sizeof(e));
                            shader->set("uEnvironment", environmentBuffer.get());
                        }

                        material = call.material;
                        if (!useMaterials) {
                            materialIndex = getTextureIndex(material->texture.get());
                        } else {
                            materialIndex = getMaterialIndex(material);
                            if (materialIndex >= materialBuffer.size()) {
                                setMaterialData(material, (MaterialData *) materialBuffer.next());
                            }
                        }
                    }

                    InstanceData *instance = (InstanceData *) instances->next();
                    instance->transform = call.transform;
                    instance->color = call.color;
                    instance->materialIndex = materialIndex;
                    instance->id = call.id;
                    instanceCount++;
                }

                if (mesh) {
                    updateLightBuffer(shader);
                    submit(mesh, shader);
                }
            }

            materialBuffer.reset();
            lightBuffer.reset();
            resetMaterials();
            resetTextures();
            resetMeshes();
            resetShaders();
        }

        void clear() {
            drawList.clear();
            calls.clear();
            lightList.clear();
        }

    };

    void Renderer::beginScene(glm::mat4 projectionMatrix, glm::vec3 eyePosition){
        drawList->projectionMatrix = projectionMatrix;
        drawList->eyePosition = eyePosition;
    }

    void Renderer::submit(const glm::mat4& transform, const glm::vec3& position, Mesh* mesh, Material* material, Color color, uint32_t id) {
        if (!material) {
            material = defaultMaterial.get();
        }
        if (!mesh) {
            mesh = quad.get();
        }
        drawList->add(transform, position, mesh, material, color, id);
    }

    void Renderer::submit(const glm::vec3 &position, const glm::vec3 direction, Light &light) {
        drawList->addLight(position, direction, light);
    }

    void Renderer::setEnvironMap(Ref<Texture> radianceMap, Ref<Texture> irradianceMap, float intensity){
        drawList->radianceMap = radianceMap;
        drawList->irradianceMap = irradianceMap;
        drawList->environmentMapIntensity = intensity;
    }

    void Renderer::drawScene(Ref<FrameBuffer> frameBuffer) {
        if (frameBuffer) {
            frameBuffer->bind();
        }
        else {
            FrameBuffer::unbind();
        }
        
        drawList->sort();
        drawList->draw(frameBuffer);

        drawCallCount += drawList->drawCallCount;
        instanceCount += drawList->instanceCount;
        meshCount += drawList->meshCount;
        materialCount += drawList->materialCount;
        shaderCount += drawList->shaderCount;
        lightCount += drawList->lightCount;
    }

    void Renderer::resetScene() {
        drawList->clear();
        drawList->radianceMap = nullptr;
        drawList->irradianceMap = nullptr;
        drawList->environmentMapIntensity = 0.0f;
    }

    void Renderer::startup() {
        RenderContext::setDepth(true);
        RenderContext::setBlend(true);
        RenderContext::setCull(true);

        quad = quad.make();
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
        quad->create(quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]), { {FLOAT, 3}, {FLOAT, 3} ,{FLOAT, 2} });

        Image image;
        image.init(1, 1, 4, 8);
        image.set(0, 0, Color::white);
        defaultTexture = defaultTexture.make();
        defaultTexture->load(image);

        defaultShader = env->assets->get<Shader>("shaders/pbr.glsl");

        defaultMaterial = defaultMaterial.make();
        defaultMaterial->texture = defaultTexture;
        defaultMaterial->shader = defaultShader;

        drawList = drawList.make();
        drawList->defaultShader = defaultShader;
        drawList->init();
    }

    void Renderer::update() {

    }

    void Renderer::shutdown() {
        defaultMaterial = nullptr;
        defaultShader = nullptr;
        defaultTexture = nullptr;
        drawList = nullptr;
    }

}
*/
