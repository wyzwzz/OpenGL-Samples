#version 430 core
layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 TexCoord;

uniform mat4 CameraMVP;

uniform mat4 LightMVP;

layout(location = 0) out vec3 outVertexPos;

layout(location = 1) out vec3 outVertexNormal;

layout(location = 2) out vec2 outVertexTexCoord;

layout(location = 3) out vec4 outPosFromLight;

void main(){
    gl_Position = CameraMVP * vec4(VertexPos,1.f);

    outVertexPos = VertexPos;

    outVertexNormal = VertexNormal;

    outVertexTexCoord = TexCoord;

    outPosFromLight = LightMVP * vec4(VertexPos,1.f);
}