//
// Created by wyz on 2022/5/16.
//
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "volume_kernel.cuh"
#include "helper_math.h"

#include <curand_kernel.h>
typedef curandStatePhilox4_32_10_t Rand_state;
#define rand(state) curand_uniform(state)

__device__ inline bool intersect_volume_box(
    float &tmin, const float3 &raypos, const float3 &raydir)
{
    const float x0 = (-0.5f - raypos.x) / raydir.x;
    const float y0 = (-0.5f - raypos.y) / raydir.y;
    const float z0 = (-0.5f - raypos.z) / raydir.z;
    const float x1 = ( 0.5f - raypos.x) / raydir.x;
    const float y1 = ( 0.5f - raypos.y) / raydir.y;
    const float z1 = ( 0.5f - raypos.z) / raydir.z;

    tmin = fmaxf(fmaxf(fmaxf(fminf(z0,z1), fminf(y0,y1)), fminf(x0,x1)), 0.0f);
    const float tmax = fminf(fminf(fmaxf(z0,z1), fmaxf(y0,y1)), fmaxf(x0,x1));
    return (tmin < tmax);
}
__device__ inline bool in_volume(
    const float3 &pos)
{
    return fmaxf(fabsf(pos.x), fmaxf(fabsf(pos.y), fabsf(pos.z))) < 0.5f;
}

__device__ inline float get_extinction(
    const VolumeKernelParams &kernel_params,
    const float3 &p)
{
        float3 pos = p + make_float3(0.5f, 0.5f, 0.5f);

        float scalar = tex3D<float>(kernel_params.volume_tex,pos.x,pos.y,pos.z);

        float4 value = tex1D<float4>(kernel_params.volume_tf_tex,scalar);
//        printf("scalar: %f, value w: %f\n",scalar,value.w);
        return value.w < 0.001f ? 0.f : kernel_params.max_extinction;

        if(scalar < 0.2f) return 0.f;
        else return kernel_params.max_extinction;
}

__device__ inline bool sample_interaction(
    Rand_state &rand_state,
    float3 &ray_pos,
    const float3 &ray_dir,
    const VolumeKernelParams &kernel_params)
{
    float t = 0.0f;
    float3 pos;
    do {
        t -= logf(max(0.1f,1.0f - rand(&rand_state))) / kernel_params.max_extinction;

        pos = ray_pos + ray_dir * t;
        if (!in_volume(pos))
            return false;

    } while (get_extinction(kernel_params, pos) < rand(&rand_state) * kernel_params.max_extinction);

    ray_pos = pos;
    return true;
}
__device__ inline float3 trace_volume(
    Rand_state &rand_state,
    float3 &ray_pos,
    float3 &ray_dir,
    const VolumeKernelParams &kernel_params){
    float t0;
    float3 w = make_float3(1.f);

    if (intersect_volume_box(t0, ray_pos, ray_dir)){
        ray_pos += ray_dir * t0;
        unsigned int num_interactions = 0;

        while (sample_interaction(rand_state, ray_pos, ray_dir, kernel_params)){
            if (num_interactions++ >= kernel_params.max_interactions)
                return make_float3(0.0f, 0.0f, 0.0f);
            float3 volume_sample_pos = ray_pos + float3{0.5f,0.5f,0.5f};
            float scalar = tex3D<float>(kernel_params.volume_tex,volume_sample_pos.x,volume_sample_pos.y,volume_sample_pos.z);
            float4 value = tex1D<float4>(kernel_params.volume_tf_tex,scalar);
            w *= make_float3(value.x,value.y,value.z);

            float t = max(w.x,max(w.y,w.z));

            if(t < 0.7f && num_interactions > 5){
                float q = max(0.05f,1.f - t);
                if(rand(&rand_state) < q)
                    break ;
                w /= 1.f - q;
            }
            // Sample isotropic phase function.
            const float phi = (float)(2.0 * M_PI) * rand(&rand_state);
            const float cos_theta = 1.0f - 2.0f * rand(&rand_state);
            const float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
            ray_dir = make_float3(
                cosf(phi) * sin_theta,
                sinf(phi) * sin_theta,
                cos_theta);
        }
    }

    const float4 texval = tex2D<float4>(
        kernel_params.env_tex,
        atan2f(ray_dir.z, ray_dir.x) * (float)(0.5 / M_PI) + 0.5f,
        acosf(fmaxf(fminf(ray_dir.y, 1.0f), -1.0f)) * (float)(1.0 / M_PI));
    return make_float3(texval.x * w.x, texval.y * w.y, texval.z * w.z);
}

__global__ void volume_rt_kernel(VolumeKernelParams kernel_params)
{
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= kernel_params.resolution.x || y >= kernel_params.resolution.y)
        return;

    const unsigned int idx = y * kernel_params.resolution.x + x;

    Rand_state rand_state;
    curand_init(idx, 0, kernel_params.iteration * 4096, &rand_state);

    const float inv_res_x = 1.0f / (float)kernel_params.resolution.x;
    const float inv_res_y = 1.0f / (float)kernel_params.resolution.y;
    const float pr = (2.0f * ((float)x + rand(&rand_state)) * inv_res_x - 1.0f);
    const float pu = (2.0f * ((float)y + rand(&rand_state)) * inv_res_y - 1.0f);
    const float aspect = (float)kernel_params.resolution.y * inv_res_x;
    float3 ray_pos = kernel_params.cam_pos;
    float3 ray_dir = normalize(kernel_params.cam_dir * kernel_params.cam_focal + kernel_params.cam_right * pr - kernel_params.cam_up * aspect * pu);


    const float3 value = trace_volume(rand_state, ray_pos, ray_dir, kernel_params);

    if (kernel_params.iteration == 0)
        kernel_params.accum_buffer[idx] = value;
    else
        kernel_params.accum_buffer[idx] = kernel_params.accum_buffer[idx] + (value - kernel_params.accum_buffer[idx]) / (float)(kernel_params.iteration + 1);

    float3 val = kernel_params.accum_buffer[idx] * kernel_params.exposure_scale;
    val.x *= (1.0f + val.x * 0.1f) / (1.0f + val.x);
    val.y *= (1.0f + val.y * 0.1f) / (1.0f + val.y);
    val.z *= (1.0f + val.z * 0.1f) / (1.0f + val.z);
    unsigned int r = (unsigned int)(255.0f * fminf(powf(fmaxf(val.x, 0.0f), (float)(1.0 / 2.2)), 1.0f));
    unsigned int g = (unsigned int)(255.0f * fminf(powf(fmaxf(val.y, 0.0f), (float)(1.0 / 2.2)), 1.0f));
    unsigned int b = (unsigned int)(255.0f * fminf(powf(fmaxf(val.z, 0.0f), (float)(1.0 / 2.2)), 1.0f));

    kernel_params.display_buffer[idx] = 0xff000000 | (r << 16) | (g << 8) | b;
}
