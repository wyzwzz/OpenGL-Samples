#version 460 core
layout(location = 0) in vec3 inVertexPos;
uniform mat4 model;
uniform mat4 view;

uniform mat4 projection;
void main() {
    gl_Position = projection * view * vec4(inVertexPos, 1.0);
}
