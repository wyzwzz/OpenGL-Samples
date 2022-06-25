file(
        GLOB
        CS_SRCS
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
        "src/*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(ClusteredShading ${CS_SRCS})

target_link_libraries(
        ClusteredShading
        PRIVATE
        base
)
target_compile_features(
        ClusteredShading
        PRIVATE
        cxx_std_17
)