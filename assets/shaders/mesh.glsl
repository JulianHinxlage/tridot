#type vertex
#version 400 core

#include "base/instanceInput.glsl"
#include "base/uEnvironment.glsl"

out vec2 fTexCoords;
flat out float fMaterialIndex;
out vec4 fColor;

void main(){
    fTexCoords = vTexCoords;
    fMaterialIndex = iMaterialIndex;
    fColor = iColor;

    gl_Position = viewProjection * iTransform * vec4(vPosition, 1.0);
}

#type fragment
#version 400 core

in vec2 fTexCoords;
flat in float fMaterialIndex;
in vec4 fColor;

#include "base/uMaterials.glsl"
#include "base/uEnvironment.glsl"

uniform sampler2D uTextures[32];

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


layout (location = 0) out vec4 oColor;

void main(){
    Material material = materials[int(fMaterialIndex)];

    vec4 albedo = material.color * fColor;
    if (material.texture != -1){
        albedo *= sampleTextureIndexed(material.texture, fTexCoords);
    }

    oColor = albedo;
}
