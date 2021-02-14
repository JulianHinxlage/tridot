#type vertex
#version 400 core

layout (location=0) in vec3 position;
layout (location=1) in vec3 color;

out vec3 fColor;

void main(){
    gl_Position = vec4(position, 1.0);
    fColor = color;
}

#type fragment
#version 400 core

in vec3 fColor;
out vec4 outColor;

void main(){
    outColor = vec4(fColor, 1.0);
}