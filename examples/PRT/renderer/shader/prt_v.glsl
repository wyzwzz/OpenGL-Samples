#version 460 core
layout(location = 0) in vec3 inVertexPos;
layout(location = 1) in vec3 inVertexNormal;
layout(location = 2) in vec2 inTexCoord;

const int SHCoeffLength = 9;

layout(std430,binding = 0) buffer LightTransport{
    float light_transport[];
}lightTransport;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 prtLight[SHCoeffLength];

layout(location = 0) out vec3 outColor;

void main(){
    gl_Position = projection * view * model * vec4(inVertexPos,1.f);
    vec3 color = vec3(0.f);
    for(int i = 0; i < SHCoeffLength; i++){
        color += prtLight[i] * lightTransport.light_transport[gl_VertexID * SHCoeffLength + i];
    }
    outColor = color;
}