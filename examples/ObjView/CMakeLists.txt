file(
        GLOB
        ObjView_SRCS
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)

add_executable(ObjView ${ObjView_SRCS})

target_link_libraries(
        ObjView
        PRIVATE
        base
)
target_compile_features(
        ObjView
        PRIVATE
        cxx_std_17
)