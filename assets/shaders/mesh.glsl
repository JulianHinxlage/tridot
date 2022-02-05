#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iTextureIndex;
layout (location=9) in vec4 iId;

out vec3 fPosition;
out vec3 fNormal;
out vec4 fColor;
out vec2 fTexCoords;
out vec4 fId;
flat out float fTextureIndex;

layout(std140) uniform uEnvironment {
    mat4 projection;
    vec3 cameraPosition;
    int align1;
    int lightCount;
    float environmentMapIntensity;
    int environmentMapIndex;
    int irradianceMapIndex;
};

void main(){
    vec4 pos = iTransform * vec4(vPosition, 1.0);
    fPosition = vec3(pos);
    gl_Position = projection * pos;
    fNormal = (iTransform * vec4(vNormal, 0.0)).xyz;
    fColor = iColor;
    fId = iId;
    fTexCoords = vTexCoords;
    fTextureIndex = iTextureIndex;
}

#type fragment
#version 400 core

in vec3 fPosition;
in vec3 fNormal;
in vec4 fColor;
in vec2 fTexCoords;
in vec4 fId;
flat in float fTextureIndex;

out vec4 oColor;
out vec4 oId;

uniform sampler2D uTextures[32];

vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords){
    if(textureIndex == -1){
        return vec4(1, 1, 1, 1);
    }
    for(int i = 0; i < 32; i++){
        if(i == textureIndex){
            return texture(uTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}

void main(){
    oColor = sampleTextureIndexed(int(fTextureIndex), fTexCoords) * fColor;
    oId = vec4(fId.xyz, 1.0);
}
