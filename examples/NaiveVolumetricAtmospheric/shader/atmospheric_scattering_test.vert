#version 430 core
layout(location = 0) in vec2 VertexPos;
void main() {
    gl_Position = vec4(VertexPos,0.f,1.f);
}
