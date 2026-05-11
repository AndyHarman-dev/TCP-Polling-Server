cmake_minimum_required(VERSION 3.14)
project(TCPEchoingServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(TCPEchoingServer main.cpp)

include(CTest)
add_executable(tests tests/test_main.cpp)
target_link_libraries(tests PRIVATE doctest::doctest)
add_test(NAME tests COMMAND tests)
