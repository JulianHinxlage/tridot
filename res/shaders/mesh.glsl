#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iMaterialIndex;
layout (location=9) in vec4 iId;

out vec3 fPosition;
out vec3 fNormal;
out vec4 fColor;
out vec4 fId;

uniform mat4 uProjection = mat4(1);

void main(){
    vec4 pos = iTransform * vec4(vPosition, 1.0);
    fPosition = vec3(pos);
    gl_Position = uProjection * pos;
    fNormal = (iTransform * vec4(vNormal, 0.0)).xyz;
    fColor = iColor;
    fId = iId;
}

#type fragment
#version 400 core

in vec3 fPosition;
in vec3 fNormal;
in vec4 fColor;
in vec4 fId;

out vec4 oColor;
//out vec4 oId;

uniform sampler2D uTextures[32];
uniform vec3 uEyePosition = vec3(0, 0, 0);


void main(){
    oColor = fColor;
    //oId = fId;
}
