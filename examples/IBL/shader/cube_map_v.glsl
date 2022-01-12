#version 460 core
layout(location = 0) in vec3 inVertexPos;

layout(location = 0) out vec3 outWorldPos;

uniform mat4 view;
uniform mat4 projection;

void main(){
    gl_Position = projection * view * vec4(inVertexPos,1.f);
    outWorldPos = inVertexPos;
}