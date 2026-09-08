cmake_minimum_required(VERSION 3.15)
project(CubeOthello VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /W4")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob1 /DNDEBUG")

# DxLib のパス設定（Windows 向け）
set(DXLIB_PATH "${CMAKE_SOURCE_DIR}/DxLib" CACHE PATH "Path to DxLib source directory")
if(NOT DXLIB_PATH)
    message(FATAL_ERROR "DxLib path not set. Set DXLIB_PATH=C:\\path\\to\\dxlib OR use -DDXLIB_PATH=\"...\"")
endif()

# メイン実行ファイルのソースファイル群
add_executable(cubo_othello
    src/main.cpp
)

target_include_directories(cubo_othello PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${DXLIB_PATH}/include
)

# DxLib ライブラリのリンク（Windows 用）
if(WIN32)
    find_library(DXLIB_LIB dxlib PATHS "${DXLIB_PATH}/lib" NO_DEFAULT_PATH)
    if(NOT DXLIB_LIB)
        message(WARNING "DxLib library not found. Building without linking dxlib.lib")
    else()
        target_link_libraries(cubo_othello PRIVATE ${DXLIB_LIB})
    endif()
endif()

# ビルドオプション
option(BUILD_TESTS "ビルド時にテストを含める" OFF)
option(DEBUG_BUILD "デバッグビルド（未最適化）" ON)
option(ENABLE_GTEST "Google Test を有効にする" OFF)
option(ENABLE_COVERAGE_REPORT "カバレッジレポート生成を有効にする" OFF)

if(DEBUG_BUILD)
    add_compile_definitions(CUBE_OTHELLO_DEBUG)
else()
    if(MSVC)
        add_compile_options(/O2 /Ob1 /DNDEBUG)
    elseif(MINGW)
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# Google Test の追加（OPTIONAL）
if(BUILD_TESTS AND ENABLE_GTEST)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

    # ユニットテストの実行ファイル
    add_executable(cubo_othello_tests tests/unit_tests.cpp)
    target_link_libraries(cubo_othello_tests PRIVATE GTest::gtest_main cubo_othello)
endif()

# カバレッジレポート生成用 Python スクリプト（OPTIONAL）
if(ENABLE_COVERAGE_REPORT)
    file(WRITE "${CMAKE_SOURCE_DIR}/tests/coverage_report.py" R"PYTHON_SCRIPT")
PYTHON_SCRIPT
#!/usr/bin/env python3
"""Cube Othello — Coverage Report Generator (E2E Tests).

Usage:
    pip install pytest pytest-cov
    ctest -C Debug --output-on-failure   # (CMake 経由で gtest を実行)
    pytest --cov=cubo_othello --cov-report=html:build/coverage/htmlcov e2e_tests.py
"""

import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).parent.parent / 'cubo_othello'
BUILD_DIR = PROJECT_ROOT / 'build'
HTML_REPORT_DIR = BUILD_DIR / 'coverage' / 'htmlcov'


def run_coverage_report() -> None:
    html_report_dir = HTML_REPORT_DIR

    if not html_report_dir.exists():
        html_report_dir.mkdir(parents=True)

    subprocess.run(
        [sys.executable, '-m', 'pytest', '--cov=cubo_othello',
         '--cov-report=html:' + str(html_report_dir),
         PROJECT_ROOT / 'tests' / 'e2e_tests.py'],
        check=False,
    )


def run_unit_tests() -> None:
    subprocess.run(
        ['ctest', '-C', 'Debug', '--output-on-failure'],
        cwd=PROJECT_ROOT / 'build', check=False,
    )


main()
PYTHON_SCRIPT

endif()

# ビルドメッセージ
message(STATUS "=== Cube Othello Build Configuration ===")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  DxLib Path:   ${DXLIB_PATH}")
message(STATUS "  Debug Build:  ${DEBUG_BUILD}")
message(STATUS "  Windows:      ${WIN32}")
message(STATUS "  Build Tests:  ${BUILD_TESTS}")
message(STATUS "  Enable GTest: ${ENABLE_GTEST}")
message(STATUS "  Coverage:     ${ENABLE_COVERAGE_REPORT}")

# インストール設定
install(TARGETS cubo_othello RUNTIME DESTINATION bin)
if(BUILD_TESTS)
    install(TARGETS cubo_othello_tests RUNTIME DESTINATION bin)
endif()
