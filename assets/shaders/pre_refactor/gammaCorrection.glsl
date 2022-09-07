#type vertex
#version 400 core

layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoords;

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
uniform vec4 uColor = vec4(1);
uniform float gamma = 2.2;
uniform float exposure = 1.0f;

out vec4 oColor;
out vec4 oId;
out vec4 oEmissive;

void main(){
    oColor = texture(uTextures[0], fTexCoords) * uColor;
    oColor.rgb = vec3(1.0) - exp(-oColor.rgb * exposure);
    oColor.rgb = pow(oColor.rgb, vec3(1.0 / gamma));
    oId = vec4(0, 0, 0, 0);
    oEmissive = vec4(0, 0, 0, 0);
}
