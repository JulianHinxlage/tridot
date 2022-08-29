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

uniform layout(RGBA8) image2D uLightBuffer;

out vec4 oColor;
out vec4 oId;

void main(){
    vec2 texCoords = gl_FragCoord.xy / textureSize(uTextures[1], 0);
    ivec2 pixelCoords = ivec2(texCoords * textureSize(uTextures[1], 0));

    vec4 input = imageLoad(uLightBuffer, pixelCoords);

    vec4 albedo = texture(uTextures[1], texCoords);
    vec4 depth = texture(uTextures[6], texCoords);
    if(depth.rgb == vec3(1, 1, 1)){
        oColor.rgb = input.rgb + albedo.rgb;
        oColor.a = albedo.a;
    
        oId.rgb = texture(uTextures[2], texCoords).rgb;
        oId.a = 1.0;
        return;
    }

    vec4 color;
    color.rgb = input.rbg + (albedo.rgb * uColor.rgb * uIntesity);
    color.a = 1.0;

    imageStore(uLightBuffer, pixelCoords, color);
    
    oColor = color;
    oId.rgb = texture(uTextures[2], texCoords).rgb;
    oId.a = 1.0;
}
