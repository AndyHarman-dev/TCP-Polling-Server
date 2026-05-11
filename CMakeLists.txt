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
# doctest v2.4.11 declares cmake_minimum_required(VERSION 2.8.12); CMake 4.x
# removed sub-3.5 compatibility so we set the floor before processing it.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(doctest)

add_library(tcp_echo_core STATIC
    util/sockets.cpp
    logging/Logger.cpp
    config/ArgParser.cpp
)
target_include_directories(tcp_echo_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TCPEchoingServer main.cpp)
target_link_libraries(TCPEchoingServer PRIVATE tcp_echo_core)

include(CTest)
add_executable(tests
    tests/test_main.cpp
    tests/test_sockets.cpp
    tests/test_logger.cpp
    tests/test_arg_parser.cpp
)
target_link_libraries(tests PRIVATE tcp_echo_core doctest::doctest)
add_test(NAME tests COMMAND tests)
