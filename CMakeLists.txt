cmake_minimum_required(VERSION 3.21)
project(CuboOthello LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
endif()

# ---------------------------------------------------------------------------
# Logic-only test target — no DXLib dependency, always builds.
# ---------------------------------------------------------------------------
find_package(GTest CONFIG REQUIRED)
enable_testing()

add_executable(cubo_tests
    src/board.cpp
    src/ai_player.cpp
    tests/board_test.cpp
    tests/ai_player_test.cpp
)
target_include_directories(cubo_tests PRIVATE src)
target_link_libraries(cubo_tests PRIVATE GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cubo_tests)

# ---------------------------------------------------------------------------
# Full game executable — only built if DXLib is available.
# ---------------------------------------------------------------------------
set(DXLIB_ROOT "${CMAKE_SOURCE_DIR}/third_party/DxLib" CACHE PATH "DXLib VC package root (contains DxLib.h)")

if(EXISTS "${DXLIB_ROOT}/DxLib.h")
    add_executable(cubo_othello WIN32
        src/main.cpp
        src/board.cpp
        src/ai_player.cpp
        src/display.cpp
        src/game_engine.cpp
    )
    target_include_directories(cubo_othello PRIVATE src "${DXLIB_ROOT}")
    target_link_directories(cubo_othello PRIVATE "${DXLIB_ROOT}")
    # windows.h (pulled in by DxLib.h) defines max()/min() macros that
    # collide with std::max/std::min and std::numeric_limits<T>::max().
    target_compile_definitions(cubo_othello PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    # DxLib.h itself selects and #pragma-comment-links the correct
    # DxLib*_x64(_d).lib variant based on _DEBUG/_WIN64; only Windows
    # multimedia timer support needs to be added explicitly.
    target_link_libraries(cubo_othello PRIVATE winmm)
else()
    message(WARNING "DXLib not found at ${DXLIB_ROOT} (expected DxLib.h there) — "
                     "skipping the cubo_othello GUI executable; only cubo_tests will build.")
endif()
