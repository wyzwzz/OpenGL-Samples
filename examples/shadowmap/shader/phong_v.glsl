#version 430 core
layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 TexCoord;
uniform mat4 CameraMVPMatrix;
uniform mat4 LightMVPMatrix;
out vec3 position;
out vec3 normal;
out vec2 tex_coord;
out vec4 pos_from_light;
void main() {
    gl_Position = CameraMVPMatrix * vec4(VertexPos,1.f);
    position = VertexPos;
    normal = VertexNormal;
    tex_coord = TexCoord;
    pos_from_light = LightMVPMatrix * vec4(VertexPos,1.f);
}
