find_package(CUDA REQUIRED)
enable_language(CUDA)
file(
        GLOB
        VPT_SRCS
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
        "src/*.hpp"
        "src/*.cu"
        "src/*.cuh"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(CUDAGLVPT ${VPT_SRCS})

target_link_libraries(
        CUDAGLVPT
        PRIVATE
        base
        ${CUDA_LIBRARIES}
)

target_include_directories(
        CUDAGLVPT
        PRIVATE
        ${CUDA_INCLUDE_DIRS}
)