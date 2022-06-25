file(
        GLOB
        ASR_SRCS
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(ASR ${ASR_SRCS})

target_link_libraries(
        ASR
        PRIVATE
        base
)
target_compile_features(
        ASR
        PRIVATE
        cxx_std_17
)