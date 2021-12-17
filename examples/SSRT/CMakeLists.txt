file(
        GLOB
        SSRT_SRCS
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
        "src/*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(SSRT ${SSRT_SRCS})
target_link_libraries(
        SSRT
        PRIVATE
        base
)
target_compile_features(
        SSRT
        PRIVATE
        cxx_std_17
)