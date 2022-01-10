file(
        GLOB
        IBL_SRCS
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
        "src/*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(IBL ${IBL_SRCS})

target_link_libraries(
        IBL
        PRIVATE
        base
        glm::glm
        spdlog::spdlog
)

target_compile_features(
        IBL
        PRIVATE
        cxx_std_17
)