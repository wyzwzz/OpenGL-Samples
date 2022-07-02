#version 460 core
layout(location = 0) in vec3 inVertexPos;

layout(location = 0) out vec3 outWorldPos;

uniform mat4 view;

uniform mat4 projection;

void main(){
    outWorldPos = inVertexPos;

    //去除view矩阵中平移的部分
    mat4 rotView = mat4(mat3(view));
    vec4 clipPos = projection * rotView * vec4(inVertexPos,1.f);
    //保证天空盒的片段深度总是为1.0 即最大值
    gl_Position = clipPos.xyww; // xyww / w ==> z = 1.0
}