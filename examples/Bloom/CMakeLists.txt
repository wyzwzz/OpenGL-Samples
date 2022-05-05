file(
        GLOB
        BLOOM_SRCS
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
        "src/*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(Bloom ${BLOOM_SRCS})

target_link_libraries(
        Bloom
        PRIVATE
        base
)
target_compile_features(
        Bloom
        PRIVATE
        cxx_std_17
)