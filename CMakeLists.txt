cmake_minimum_required(VERSION 3.17)
PROJECT(OpenGL-Samples
        LANGUAGES C CXX)


add_subdirectory(base)

add_subdirectory(examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)