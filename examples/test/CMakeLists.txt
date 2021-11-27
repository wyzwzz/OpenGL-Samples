
add_executable(test_glcontext test_glcontext.cpp)
target_link_libraries(test_glcontext
        PRIVATE
        base
        )
target_compile_features(
        test_glcontext
        PRIVATE
        cxx_std_17
)