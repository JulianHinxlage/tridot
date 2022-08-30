#type vertex
#version 400 core

#include "base/vertexInput.glsl"

layout (location=3) in mat4 iTransform;
layout (location=7) in vec3 iPosition;
layout (location=8) in vec4 iColor;
layout (location=9) in float iIntesity;
layout (location=10) in float iRange;
layout (location=11) in float iFalloff;

uniform mat4 uProjection = mat4(1);

flat out vec3 fPosition;
flat out vec4 fColor;
flat out float fIntesity;
flat out float fRange;
flat out float fFalloff;

void main(){
    gl_Position = uProjection * iTransform * vec4(vPosition, 1.0);
    fPosition = iPosition;
    fColor = iColor;
    fIntesity = iIntesity;
    fRange = iRange;
    fFalloff = iFalloff;
}

#type fragment
#version 420 core

#include "base/pbr.glsl"

uniform sampler2D uTextures[32];
uniform vec3 uEyePosition = vec3(0);

flat in vec3 fPosition;
flat in vec4 fColor;
flat in float fIntesity;
flat in float fRange;
flat in float fFalloff;

out vec4 oColor;

void main(){
    vec2 texCoords = gl_FragCoord.xy / textureSize(uTextures[0], 0);
    ivec2 pixelCoords = ivec2(texCoords * textureSize(uTextures[0], 0));

    vec4 albedo = texture(uTextures[0], texCoords);
    vec4 depth = texture(uTextures[4], texCoords);
    if(depth.rgb == vec3(1, 1, 1)){
        oColor = vec4(0, 0, 0, 0);
        return;
    }

    vec3 normal = (texture(uTextures[1], texCoords).rgb * 2.0f) - 1.0f;
    vec3 position = texture(uTextures[2], texCoords).rgb;
    vec3 rme = texture(uTextures[3], texCoords).rgb;
    float metallic = rme.g;
    float roughness = rme.r;

    
    vec3 lightDirection = normalize(fPosition - position);
    float lightDistance = length(fPosition - position);
    float attenuation = 1.0 / pow(lightDistance + 1, fFalloff);
    attenuation *= max(0.0, min(1.0, (fRange - lightDistance) * fRange * 0.1));

    vec3 viewDirection = normalize(uEyePosition - position);
    float ndotl = max(0.0, dot(normal, lightDirection));

    vec3 radiance = fColor.rgb * fIntesity;
    vec4 color;
    color.rgb = pbrLighting(albedo.rgb, normal, viewDirection, lightDirection, metallic, roughness) * radiance * attenuation;
    color.a = 1.0;

    oColor = color;
}
