cmake_minimum_required(VERSION 3.16)
project(CuboOthello VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# DXLib include directory (environment variable or local copy)
find_path(DXLIB_INCLUDE_DIR dxlib.h
    PATHS ${DXLIB_PATH} $ENV{DXLIB_PATH} /dxlib_libs/include
    PATH_SUFFIXES lib/include)

if(NOT DXLIB_INCLUDE_DIR)
    # Fallback: if DXLib is not installed, use mock headers for unit testing
    message(STATUS "DXLib not found — building in test-only mode (no rendering).")
    add_library(dxlib_stub INTERFACE)
    target_include_directories(dxlib_stub INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/mock_dxlib)
endif()

find_package(GTest CONFIG REQUIRED)
enable_testing()

# --- gtest executables ---
add_executable(cubo_board_test tests/cubo_othello/board_test.cpp)
target_link_libraries(cubo_board_test PRIVATE GTest::gtest GTest::gtest_main dxlib_stub)

include(CTest)
add_test(NAME cubo_board_test COMMAND cubo_board_test)
