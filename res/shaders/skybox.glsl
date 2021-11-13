#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iMaterialIndex;
layout (location=9) in vec4 iId;


out vec3 fLocalPosition;

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
    fLocalPosition = vPosition;
    gl_Position = projection * iTransform * vec4(normalize(vPosition), 1.0);
}

#type fragment
#version 400 core

in vec3 fLocalPosition;

out vec4 oColor;

uniform samplerCube uEnvironmentMap;

void main(){
    vec3 v = fLocalPosition;
    oColor = texture(uEnvironmentMap, vec3(v.x, -v.z, v.y));
}
