cmake_minimum_required(VERSION 3.20)

project(CppReferenceExtractor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CppReferenceExtractor
    src/main.cpp
    src/CppReferenceExtractor.cpp
)

target_include_directories(CppReferenceExtractor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Test executable
add_executable(CppReferenceExtractorTest
    test/test_main.cpp
    src/CppReferenceExtractor.cpp
)

target_include_directories(CppReferenceExtractorTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
)