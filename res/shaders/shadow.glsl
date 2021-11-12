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
    gl_Position = projection * iTransform * vec4(vPosition, 1.0);
}

#type fragment
#version 400 core

void main(){}
