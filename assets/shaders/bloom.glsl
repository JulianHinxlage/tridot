#type vertex
#version 400 core

#include "base/vertexInput.glsl"

uniform mat4 uProjection = mat4(1);
uniform mat4 uTransform = mat4(1);

out vec2 fTexCoords;

void main(){
    gl_Position = uProjection * uTransform * vec4(vPosition, 1.0);
    fTexCoords = vTexCoords;
}

#type fragment
#version 400 core

in vec2 fTexCoords;

uniform sampler2D uTextures[32];
uniform float bloomThreshold = 1;

out vec4 oColor;

void main(){
    vec2 texCoords = gl_FragCoord.xy / textureSize(uTextures[0], 0);

    vec4 albedo = texture(uTextures[0], texCoords);
    vec4 depth = texture(uTextures[4], texCoords);
    if(depth.rgb == vec3(1, 1, 1)){
        oColor = vec4(0, 0, 0, 0);
        return;
    }

    vec3 rme = texture(uTextures[3], texCoords).rgb;
    float emissive = rme.b;

    vec4 lightOutput = texture(uTextures[5], texCoords);

    float brightness = dot(lightOutput.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > bloomThreshold){
        oColor = vec4(lightOutput.rgb * (brightness - bloomThreshold), 1.0);
    }else{
        oColor = vec4(0.0, 0.0, 0.0, emissive > 0.0 ? 1.0 : albedo.a);
    }
    oColor.rgb += albedo.rgb * emissive;
    oColor.a = 1.0;
}
