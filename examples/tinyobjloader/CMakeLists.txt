
add_executable(obj-viewer main.cpp)

target_link_libraries(
        obj-viewer
        PRIVATE
        tinyobjloader
        base
        spdlog::spdlog
        glm::glm
)

target_compile_features(
        obj-viewer
        PRIVATE
        cxx_std_17
)