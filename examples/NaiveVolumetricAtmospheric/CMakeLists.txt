file(
        GLOB
        NVA_SRCS
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
        "src/*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(NaiveVolumetricAtmospheric ${NVA_SRCS})

target_link_libraries(
        NaiveVolumetricAtmospheric
        PRIVATE
        base
)
target_compile_features(
        NaiveVolumetricAtmospheric
        PRIVATE
        cxx_std_17
)