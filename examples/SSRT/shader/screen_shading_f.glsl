#version 430 core

layout(binding = 0,rgba32f) uniform image2D GBufferDiffuse;

layout(binding = 1,rgba32f) uniform image2D GBufferPos;

layout(binding = 2,rgba32f) uniform image2D GBufferNormal;

layout(binding = 3, r32f) uniform image2D GBufferDepth;

layout(binding = 4, r32f) uniform image2D GBufferShadow;

layout(binding = 5, rgba8) uniform image2D DirectColor;

layout(binding = 6, rgba8) uniform image2D IndirectColor;

layout(binding = 7, rgba8) uniform image2D CompositeColor;

layout(location = 0) out vec4 FragColor;

uniform int DrawModel;

void main() {
    if(DrawModel == 0){
        FragColor = vec4(imageLoad(GBufferDiffuse,ivec2(gl_FragCoord)).rgb,1.f);
    }
    else if(DrawModel == 1){
        FragColor = vec4(imageLoad(GBufferPos,ivec2(gl_FragCoord)).rgb,1.f);
    }
    else if(DrawModel == 2){
        FragColor = vec4(imageLoad(GBufferNormal,ivec2(gl_FragCoord)).rgb,1.f);
    }
    else if(DrawModel == 3){
        float depth = imageLoad(GBufferDepth,ivec2(gl_FragCoord)).r;
        FragColor = vec4(depth,depth,depth,1.f);
    }
    else if(DrawModel == 4){
        float shadow = imageLoad(GBufferShadow,ivec2(gl_FragCoord)).r;
        FragColor = vec4(shadow,shadow,shadow,1.f);
    }
    else if(DrawModel == 5){
        FragColor = vec4(imageLoad(DirectColor,ivec2(gl_FragCoord)).rgb,1.f);
    }
    else if(DrawModel == 6){
        FragColor = vec4(imageLoad(IndirectColor,ivec2(gl_FragCoord)).rgb,1.f);
    }
    else if(DrawModel == 7){
        FragColor = imageLoad(CompositeColor,ivec2(gl_FragCoord));
    }
    else{
        FragColor = vec4(0.f,0.f,0.f,1.f);
    }
}
