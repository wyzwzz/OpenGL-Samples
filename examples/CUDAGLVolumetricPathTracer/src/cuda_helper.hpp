//
// Created by wyz on 2021/6/10.
//

#pragma once

#include <cuda.h>
#include <cuda_runtime.h>

#include <iostream>

inline bool check(CUresult e, int iLine, const char *szFile)
{
    if (e != CUDA_SUCCESS)
    {
        const char *szErrName = NULL;
        cuGetErrorName(e, &szErrName);
//        LOG_ERROR("CUDA driver API error: {0} at line {1:d} in file {2}", szErrName, iLine, szFile);
        std::cerr<<"CUDA driver API error: "<<szErrName<<" at line "<<iLine<<" in file "<<szFile<<std::endl;
        return false;
    }
    return true;
}
inline bool check(cudaError_t e, int line, const char *file)
{
    if (e != cudaSuccess)
    {
        const char *error_name = nullptr;
        error_name = cudaGetErrorName(e);
//        LOG_ERROR("CUDA runtime API error: {0} at line {1:d} in file {2}", error_name, line, file);
        std::cerr<<"CUDA runtime API error: "<<error_name<<" at line "<<line<<" in file "<<file<<std::endl;
        return false;
    }
    return true;
}

inline bool check(int line, const char *file)
{
    auto err = cudaGetLastError();
    return check(err, line, file);
}

#define CUDA_DRIVER_API_CALL(call) check(call, __LINE__, __FILE__)

#define checkCUDAErrors(call) check(call, __LINE__, __FILE__)

#define CUDA_RUNTIME_API_CALL(call) check(call, __LINE__, __FILE__)

#define CUDA_RUNTIME_CHECK check(__LINE__, __FILE__);
