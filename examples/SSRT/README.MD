# Screen Space Ray Tracing

绘制一帧的步骤
* 从光源处渲染看到的深度图 Depth-Framebuffer & Single render pass
* 从相机处渲染GBuffer(pos norma and depth) Color-Framebuffer & subpass 0
* 生成上一步得到的深度图的mipmap Color-Framebuffer & subpass 1
* 直接光着色(考虑阴影 使用BRDF) Color-Framebuffer & subpass 2
* 间接光着色(RayMarching) Color-Framebuffer & subpass 3
* 直接光和间接光结果混合 Color-Framebuffer & subpass 4
* 最终结果渲染(可以选择是否加入间接光) Color-Framebuffer & subpass 5