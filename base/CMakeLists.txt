include(${PROJECT_SOURCE_DIR}/third_party/tinyobjloader.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/spdlog.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/glm.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/imgui.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/tinygltf.cmake)
file(
        GLOB
        BASE_SRC
        "*.cpp"
        "*.c"
        "*.hpp"
        "*.h"
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)
file(
        GLOB
        BASE_HEADERS
        "*.h"
        "*.hpp"
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(${PROJECT_SOURCE_DIR}/third_party/glfw.cmake)
add_library(base STATIC ${BASE_SRC})
target_link_libraries(
        base
        PUBLIC
        glfw
        tinyobjloader
        glm
        spdlog::spdlog
        imgui
        tinygltf
)
target_include_directories(
        base
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/base>
#        $<INSTALL_INTERFACE:base>
        ${PROJECT_SOURCE_DIR}/third_party/glad/include
)

target_compile_features(
        base PUBLIC cxx_std_17
)