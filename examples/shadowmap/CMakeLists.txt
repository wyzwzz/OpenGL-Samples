set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(ShaderMap main.cpp)

target_link_libraries(ShaderMap base)

target_compile_features(
    ShaderMap PRIVATE cxx_std_17
)