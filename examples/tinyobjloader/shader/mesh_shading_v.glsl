#version 430 core
layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
uniform mat4 MVPMatrix;
void main() {
    gl_Position=MVPMatrix*vec4(VertexPos,1.f);
}
