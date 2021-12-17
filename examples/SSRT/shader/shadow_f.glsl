#version 430 core

layout(location = 0) out float outFragColor;

void main(){
    outFragColor = gl_FragCoord.z;
}