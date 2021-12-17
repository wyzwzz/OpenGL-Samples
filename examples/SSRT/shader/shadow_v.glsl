#version 430 core
layout(location = 0) in vec3 VertexPos;

uniform mat4 LightMVP;

void main(){
    gl_Position = LightMVP * vec4(VertexPos,1.f);
}