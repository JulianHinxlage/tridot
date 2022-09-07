#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

layout (location=3) in mat4 iTransform;
layout (location=7) in vec4 iColor;
layout (location=8) in float iTextureIndex;
layout (location=9) in vec4 iId;

uniform mat4 uProjection = mat4(1);

out vec2 fTexCoords;
out vec4 fColor;
flat out float fTextureIndex;
out vec4 fId;

void main(){
    gl_Position = uProjection * iTransform * vec4(vPosition, 1.0);
    fTexCoords = vTexCoords;
    fColor = iColor;
    fTextureIndex = iTextureIndex;
    fId = iId;
}

#type fragment
#version 400 core

in vec2 fTexCoords;
in vec4 fColor;
flat in float fTextureIndex;
in vec4 fId;

uniform sampler2D uTextures[32];

out vec4 oColor;
out vec4 oId;

void main(){
    oColor = texture(uTextures[int(fTextureIndex)], fTexCoords) * fColor;
    oId = vec4(fId.rgb, 1);
}
