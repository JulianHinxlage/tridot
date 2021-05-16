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
uniform float tint = 0.1;

vec4 sampleTextureIndexed(int textureIndex, vec2 textureCoords){
    for(int i = 0; i < 32; i++){
        if(i == textureIndex){
            return texture(uTextures[i], textureCoords);
        }
    }
    return vec4(0, 0, 0, 1);
}

void main(){
    vec4 color = sampleTextureIndexed(int(fTextureUnit), fTexCoords);
    vec3 gray = vec3(color.r + color.g + color.b) / 3.0f;
    oColor = mix(vec4(gray, color.a), color, tint);
}
