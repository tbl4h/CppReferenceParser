cmake_minimum_required(VERSION 3.20)

project(CppReferenceExtractor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CppReferenceExtractor
    src/main.cpp
    src/CppReferenceExtractor.cpp
)

target_include_directories(CppReferenceExtractor
    PRIVATE
        include
        /opt/lexbor/include
)

find_package(lexbor CONFIG REQUIRED
    PATHS /opt/lexbor/lib/cmake/lexbor
    NO_DEFAULT_PATH
)

target_link_libraries(CppReferenceExtractor
    PRIVATE
        lexbor::lexbor
)