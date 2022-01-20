#version 460 core
layout(location = 0) in vec3 inColor;
layout(location = 0) out vec4 outFragColor;

void main(){
    outFragColor = vec4(pow(inColor,vec3(1.f/2.2)), 1.f);
}