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
    src/game_engine.cpp
    tests/board_test.cpp
    tests/ai_player_test.cpp
    tests/game_engine_test.cpp
)
target_include_directories(cubo_tests PRIVATE src)
target_link_libraries(cubo_tests PRIVATE GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cubo_tests)

# ---------------------------------------------------------------------------
# Coverage report (NFR-003). gcov/lcov are GCC-toolchain tools and do not
# apply to this MSVC build; OpenCppCoverage is the Windows/MSVC equivalent
# and produces a comparable HTML report.
# https://github.com/OpenCppCoverage/OpenCppCoverage
# ---------------------------------------------------------------------------
find_program(OPENCPPCOVERAGE_EXE OpenCppCoverage.exe
    PATHS "$ENV{ProgramFiles}/OpenCppCoverage" "$ENV{ProgramFiles\(x86\)}/OpenCppCoverage")

if(OPENCPPCOVERAGE_EXE)
    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/src" COVERAGE_SOURCES_NATIVE)
    file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/coverage_report" COVERAGE_OUTPUT_NATIVE)

    add_custom_target(coverage
        COMMAND "${OPENCPPCOVERAGE_EXE}"
                --quiet
                --sources "${COVERAGE_SOURCES_NATIVE}"
                --export_type "html:${COVERAGE_OUTPUT_NATIVE}"
                -- "$<TARGET_FILE:cubo_tests>"
        DEPENDS cubo_tests
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running cubo_tests under OpenCppCoverage -> build/coverage_report/index.html"
        VERBATIM
    )
else()
    message(STATUS "OpenCppCoverage.exe not found — 'coverage' target not available. "
                    "Install it (winget install OpenCppCoverage.OpenCppCoverage) to enable it.")
endif()

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
        src/game_loop.cpp
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

    # -----------------------------------------------------------------------
    # Performance benchmark (T031/T032) — manual run, not part of ctest.
    # -----------------------------------------------------------------------
    add_executable(cubo_perf_benchmark WIN32
        tests/perf_benchmark.cpp
        src/board.cpp
        src/display.cpp
    )
    target_include_directories(cubo_perf_benchmark PRIVATE src "${DXLIB_ROOT}")
    target_link_directories(cubo_perf_benchmark PRIVATE "${DXLIB_ROOT}")
    target_compile_definitions(cubo_perf_benchmark PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(cubo_perf_benchmark PRIVATE winmm psapi)
else()
    message(WARNING "DXLib not found at ${DXLIB_ROOT} (expected DxLib.h there) — "
                     "skipping the cubo_othello GUI executable; only cubo_tests will build.")
endif()
