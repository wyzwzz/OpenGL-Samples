#version 460 core
layout(location = 0) in vec3 inVertexPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

void main(){
    gl_Position = vec4(inVertexPos,1.f);

    outTexCoord = inTexCoord;
}