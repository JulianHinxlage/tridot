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
out vec2 fTexCoords;
flat out float fTextureUnit;

uniform mat4 uProjection = mat4(1);

void main(){
    gl_Position = uProjection * iTransform * vec4(vPosition, 1.0);
    fColor = iColor;
    fTexCoords = vTexCoords * (iTexCoordsBottomRight - iTexCoordsTopLeft) + iTexCoordsTopLeft;
    fTextureUnit = iTextureUnit;
}

#type fragment
#version 400 core

in vec4 fColor;
in vec2 fTexCoords;
flat in float fTextureUnit;
out vec4 oColor;

uniform sampler2D uTextures[32];
uniform vec3 uCameraPosition = vec3(0, 0, 0);
uniform int steps = 10;
uniform vec2 spread = vec2(0.001, 0);

vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords){
    for(int i = 0; i < 32; i++){
        if(i == textureIndex){
            return texture(uTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}

void main(){
    vec4 color;
    for(int i = -steps; i <= steps; i++){
        color += sampleTextureIndexed(int(fTextureUnit), clamp(fTexCoords + i * spread, 0, 0.999));
    }
    oColor = color / (steps * 2 + 1);
}
