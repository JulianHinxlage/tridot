#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iMaterialIndex;
layout (location=9) in vec4 iId;

layout(std140) uniform uEnvironment {
    mat4 projection;
    mat4 view;
    mat4 viewProjection;
    vec3 eyePosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int radianceMapIndex;
    int irradianceMapIndex;
};

out vec3 fLocalPosition;
out vec3 fLocalNormal;
out vec2 fTexCoords;
flat out float fMaterialIndex;
out vec4 fColor;
out vec3 fPosition;
out vec3 fNormal;
out vec3 fScale;
out vec4 fId;

void main(){
    fLocalPosition = vPosition;
    fLocalNormal = vNormal;
    fTexCoords = vTexCoords;

    fMaterialIndex = iMaterialIndex;
    fColor = iColor;
    fId = iId;

    vec4 pos = iTransform * vec4(vPosition, 1.0);
    fPosition = pos.xyz;
    gl_Position = viewProjection * pos;
    fScale = vec3(length(iTransform[0].xyz), length(iTransform[1].xyz), length(iTransform[2].xyz));
    fNormal = normalize(vec3(transpose(inverse(iTransform)) * vec4(vNormal, 1.0)));
}

#type fragment
#version 400 core

in vec3 fLocalPosition;
in vec3 fLocalNormal;
in vec2 fTexCoords;
flat in float fMaterialIndex;
in vec4 fColor;
in vec3 fPosition;
in vec3 fNormal;
in vec3 fScale;
in vec4 fId;

struct Material{
    vec4 color;
    int mapping;
    float roughness;
    float metallic;
    float normalMapFactor;
    float emissive;
    int texture;
    int normalMap;
    int roughnessMap;
    int metallicMap;
    int ambientOcclusionMap;
    int displacementMap;
    int align1;
    vec2 textureOffset;
    vec2 textureScale;
    vec2 normalMapOffset;
    vec2 normalMapScale;
    vec2 roughnessMapOffset;
    vec2 roughnessMapScale;
    vec2 metallicMapOffset;
    vec2 metallicMapScale;
    vec2 ambientOcclusionMapOffset;
    vec2 ambientOcclusionMapScale;
    vec2 displacementMapOffset;
    vec2 displacementMapScale;
};
layout(std140) uniform uMaterials {
    Material materials[32];
};

layout(std140) uniform uEnvironment {
    mat4 projection;
    mat4 view;
    mat4 viewProjection;
    vec3 eyePosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int radianceMapIndex;
    int irradianceMapIndex;
};

uniform sampler2D uTextures[32];
const float PI = 3.14159265359;

layout (location = 0) out vec4 oAlbedo;
layout (location = 1) out vec4 oId;
layout (location = 2) out vec4 oNormal;
layout (location = 3) out vec4 oPosition;
layout (location = 4) out vec4 oRoughnessMetallicEmissive;

vec4 sampleTexture(int textureIndex, int mapping, vec2 textureScale, vec2 textureOffset);
vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords);

void main(){
    Material material = materials[int(fMaterialIndex)];

    //albedo
    vec4 albedo = material.color * fColor;
    if (material.texture != -1){
        albedo *= sampleTexture(material.texture, material.mapping, material.textureScale, material.textureOffset);
    }

    //normal
    vec3 normal = fNormal;
    if(material.normalMap != -1){
        vec3 n = sampleTexture(material.normalMap, material.mapping, material.normalMapScale, material.normalMapOffset).rgb;
        n = normalize(n * 2.0 - 1.0);
        vec3 tangent = normalize(n - normal * dot(n, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 tbn = mat3(tangent, bitangent, normal);
        normal = normalize(tbn * n) * material.normalMapFactor + normal * (1.0 - material.normalMapFactor);
    }

    //roughness
    float roughness = material.roughness;
    if(material.roughnessMap != -1){
        roughness *= (sampleTexture(material.roughnessMap, material.mapping, material.roughnessMapScale, material.roughnessMapOffset).r);
    }

    //metallic
    float metallic = material.metallic;
    if(material.metallicMap != -1){
        metallic *= (sampleTexture(material.metallicMap, material.mapping, material.metallicMapScale, material.metallicMapOffset).r);
    }

    //ambient occlusion
    float ao = 1;
    if(material.ambientOcclusionMap != -1){
        ao *= (sampleTexture(material.ambientOcclusionMap, material.mapping, material.ambientOcclusionMapScale, material.ambientOcclusionMapOffset).r);
    }

    //metallic
    float displacement = 0;
    if(material.displacementMap != -1){
        displacement = (sampleTexture(material.displacementMap, material.mapping, material.displacementMapScale, material.displacementMapOffset).r);
    }


    //outputs
    oAlbedo = albedo;
    oId = vec4(fId.xyz, 1.0);
    
    //world space
    oNormal = vec4(normal + 1.0 * 0.5, 1.0);
    oPosition = vec4(fPosition, 1.0);

    oRoughnessMetallicEmissive = vec4(roughness, metallic, material.emissive, 1.0);
}

vec4 sampleTexture(int textureIndex, int mapping, vec2 textureScale, vec2 textureOffset){
    vec4 color = vec4(1, 1, 1, 1);
    if(mapping == 0){
        //uv mapping
        color = sampleTextureIndexed(textureIndex, fTexCoords * textureScale + textureOffset);
    }else if(mapping == 1){
        //tri planar mapping
        vec3 n = abs(normalize(fLocalNormal));
        n = pow(n, vec3(1, 1, 1) * 3.0f);
        color =
        sampleTextureIndexed(textureIndex, fLocalPosition.xy * textureScale + textureOffset) * n.z +
        sampleTextureIndexed(textureIndex, fLocalPosition.xz * textureScale + textureOffset) * n.y +
        sampleTextureIndexed(textureIndex, fLocalPosition.yz * textureScale + textureOffset) * n.x;
        color /= n.x + n.y + n.z;
    }else if(mapping == 2){
        //scaled tri planar mapping
        vec3 n = abs(normalize(fLocalNormal));
        n = pow(n, vec3(1, 1, 1) * 3.0f);
        color =
        sampleTextureIndexed(textureIndex, fLocalPosition.xy * fScale.xy * textureScale + textureOffset) * n.z +
        sampleTextureIndexed(textureIndex, fLocalPosition.xz * fScale.xz * textureScale + textureOffset) * n.y +
        sampleTextureIndexed(textureIndex, fLocalPosition.yz * fScale.yz * textureScale + textureOffset) * n.x;
        color /= n.x + n.y + n.z;
    }
    return color;
}

vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords){
    if(textureIndex == -1){
        return vec4(1, 1, 1, 1);
    }
    for(int i = 0; i < 30; i++){
        if(i == textureIndex){
            return texture(uTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}