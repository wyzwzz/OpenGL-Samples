
add_executable(obj-viewer main.cpp)

target_link_libraries(
        obj-viewer
        PRIVATE
        tinyobjloader
        base
        spdlog::spdlog
        glm::glm
)