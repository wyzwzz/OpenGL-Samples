//
// Created by wyz on 2022/5/16.
//

#ifndef OPENGL_SAMPLES_VOLUME_KERNEL_CUH
#define OPENGL_SAMPLES_VOLUME_KERNEL_CUH
#include <cstdint>
struct VolumeKernelParams{
    uint32_t* display_buffer;
    uint2 resolution;
    float exposure_scale;

    float3* accum_buffer;
    uint32_t iteration;
    uint32_t max_interactions;

    float3 cam_pos;
    float3 cam_dir;
    float3 cam_right;
    float3 cam_up;
    float  cam_focal;

    cudaTextureObject_t env_tex;

    cudaTextureObject_t volume_tex;
    cudaTextureObject_t volume_tf_tex;

    float max_extinction;
};

__global__ void volume_rt_kernel(VolumeKernelParams kernel_params);

#endif // OPENGL_SAMPLES_VOLUME_KERNEL_CUH
