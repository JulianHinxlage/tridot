#type vertex
#version 400 core

#include "base/vertexInput.glsl"

uniform mat4 uProjection = mat4(1);
uniform mat4 uTransform = mat4(1);

void main(){
    gl_Position = uProjection * uTransform * vec4(vPosition, 1.0);
}

#type fragment
#version 420 core

uniform sampler2D uTextures[32];
uniform vec4 uColor = vec4(1);
uniform float uIntesity = 1.0;

out vec4 oColor;

void main(){
    vec2 texCoords = gl_FragCoord.xy / textureSize(uTextures[0], 0);
    ivec2 pixelCoords = ivec2(texCoords * textureSize(uTextures[0], 0));

    vec4 albedo = texture(uTextures[0], texCoords);
    vec4 depth = texture(uTextures[4], texCoords);
    float ao = texture(uTextures[5], texCoords).r;
    if(depth.rgb == vec3(1, 1, 1)){
        oColor = albedo;
        return;
    }

    oColor.rgb = albedo.rgb * uColor.rgb * uIntesity * ao;
    oColor.a = 1.0;
}
