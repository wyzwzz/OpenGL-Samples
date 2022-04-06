#version 430 core

layout(binding = 5, rgba8) uniform image2D DirectColor;

layout(binding = 6, rgba8) uniform image2D IndirectColor;

layout(location = 0) out vec4 CompositeColor;
#define INV_PI 0.31830988618
void main() {
    vec4 direct_color = imageLoad(DirectColor,ivec2(gl_FragCoord));
    vec4 indirect_color = imageLoad(IndirectColor,ivec2(gl_FragCoord));
    CompositeColor = direct_color*direct_color.a + indirect_color*indirect_color.a;
    CompositeColor.a = 1.f;
}
