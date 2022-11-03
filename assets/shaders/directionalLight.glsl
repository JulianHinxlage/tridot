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

#include "base/sampleTextureIndexed.glsl"

uniform vec4 uColor = vec4(1);
uniform float uIntesity = 1.0;
uniform vec3 uDirection = vec3(0.5, 0.25, 0.125);
uniform vec3 uEyePosition = vec3(0);
uniform mat4 uLightProjection = mat4(1);

out vec4 oColor;

float shadowMapping(float ndotl, vec3 fPosition){
    vec3 pos = vec3(uLightProjection * vec4(fPosition, 1.0)) * 0.5 + 0.5;

    float bias = max(0.01 * (1.0 - ndotl), 0.001);
    if(pos.z > 1.0){
        pos.z = 1.0;
    }

    vec2 texelSize = 1.0 / textureSize(uTextures[0], 0);
    float shadow = 0;
    int count = 2;
    for(int x = -count; x <= count; ++x){
        for(int y = -count; y <= count; ++y){
            vec2 uv = pos.xy + vec2(x, y) * texelSize;
            float depth = sampleTextureIndexed(5, uv).r;
            float cull = float(uv != clamp(uv, 0, 1));
            shadow += (depth + bias) < pos.z ? cull : 1.0;
        }
    }

    return shadow / float((count * 2 + 1) * (count * 2 + 1));
}

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

    vec3 radiance = uColor.rgb * uIntesity * shadowMapping(ndotl, position);
    oColor.rgb = pbrLighting(albedo.rgb, normal, viewDirection, lightDirection, metallic, roughness) * radiance;
    oColor.a = 1.0;
}
