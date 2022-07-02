file(
        GLOB_RECURSE
        ColorGrading_SRCS
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(ColorGrading ${ColorGrading_SRCS})

target_link_libraries(
        ColorGrading
        PRIVATE
        base
)

target_compile_features(
        ColorGrading
        PRIVATE
        cxx_std_17
)