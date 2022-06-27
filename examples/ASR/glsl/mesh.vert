#version 460 core

layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 TexCoord;

uniform mat4 Model;
uniform mat4 NormalModel;
uniform mat4 VP;

layout(location = 0) out vec3 oVertexPos;
layout(location = 1) out vec3 oVertexNormal;
layout(location = 2) out vec2 oTexCoord;

void main() {
    gl_Position = VP * Model * (vec4(VertexPos, 1.0));
    oVertexPos = vec3(Model * vec4(VertexPos,1.f));
    oVertexNormal = vec3(NormalModel * vec4(VertexNormal,0.0));
    oTexCoord = TexCoord;
}
