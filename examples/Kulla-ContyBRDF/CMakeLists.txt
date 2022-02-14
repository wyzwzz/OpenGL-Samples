add_subdirectory(lut-gen)

add_executable(Kulla-ContyBRDF src/main.cpp)

target_link_libraries(Kulla-ContyBRDF
    PRIVATE
    base
    )

target_compile_features(
    Kulla-ContyBRDF
        PRIVATE
        cxx_std_17
)