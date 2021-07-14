#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iTextureUnit;
layout (location=9) in vec2 iTexCoordsTopLeft;
layout (location=10) in vec2 iTexCoordsBottomRight;

out vec4 fColor;
out vec3 fLocalPosition;
out vec3 fLocalNormal;
out vec2 fTexCoords;
flat out float fTextureUnit;

uniform mat4 uProjection = mat4(1);

void main(){
    fLocalPosition = vPosition;
    vec4 position = uProjection * iTransform * vec4(normalize(vPosition), 1.0);
    gl_Position = position;
    fColor = iColor;
    fTexCoords = vTexCoords * (iTexCoordsBottomRight - iTexCoordsTopLeft) + iTexCoordsTopLeft;
    fTextureUnit = iTextureUnit;
    fLocalNormal = vNormal;
}

#type fragment
#version 400 core

in vec4 fColor;
in vec3 fLocalPosition;
in vec3 fLocalNormal;
in vec2 fTexCoords;
flat in float fTextureUnit;
out vec4 oColor;

uniform sampler2D uTextures[31];
uniform samplerCube uEnvironmentMap;
uniform vec3 uCameraPosition = vec3(0, 0, 0);

void main(){
    oColor = texture(uEnvironmentMap, fLocalPosition);
}
