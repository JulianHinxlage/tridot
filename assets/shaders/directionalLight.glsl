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

#include "base/pbr.glsl"

uniform sampler2D uTextures[32];
uniform vec4 uColor = vec4(1);
uniform float uIntesity = 1.0;
uniform vec3 uDirection = vec3(0.5, 0.25, 0.125);
uniform vec3 uEyePosition = vec3(0);

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

    vec3 viewDirection = normalize(uEyePosition - position);
    vec3 lightDirection = normalize(-uDirection);
    float ndotl = max(0.0, dot(normal, lightDirection));

    vec3 radiance = uColor.rgb * uIntesity;
    oColor.rgb = pbrLighting(albedo.rgb, normal, viewDirection, lightDirection, metallic, roughness) * radiance;
    oColor.a = 1.0;
}
