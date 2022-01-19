
add_executable(PRT src/main.cpp)
target_link_libraries(PRT
    PRIVATE
    base
    )

target_compile_features(
    PRT
        PRIVATE
        cxx_std_17
)