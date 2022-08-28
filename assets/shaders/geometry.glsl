#type vertex
#version 400 core

#include "base/instanceInput.glsl"
#include "base/uEnvironment.glsl"

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

#include "base/uMaterials.glsl"
#include "base/uEnvironment.glsl"
#include "base/sampleTexture.glsl"
#include "base/geometryOutput.glsl"

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
