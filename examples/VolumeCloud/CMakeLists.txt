file(
        GLOB_RECURSE
        VolumeCloud_SRCS
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(VolumeCloud ${VolumeCloud_SRCS})

target_link_libraries(
        VolumeCloud
        PRIVATE
        base
)

target_compile_features(
        VolumeCloud
        PRIVATE
        cxx_std_17
)