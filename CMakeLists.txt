cmake_minimum_required(VERSION 3.15)
project(cubo_othello VERSION 0.1 LANGUAGES CXX)

# ============================================
# 共通設定
# ============================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ビルドタイプ（Debug / Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# コンパイラ設定：警告を有効化
option(DEBUG_FLAGS "Enable debug flags (-Wall -Wextra)" ON)
if(DEBUG_FLAGS)
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

# ============================================
# DXLib ライブラリの設定
# ============================================
# DXLib の include ディレクトリとライブラリのパスを指定
# Windows 版：dxlib_libs\include と dxlib_libs\lib
find_path(DXLIB_INCLUDE_DIR
    NAMES dxlib.h
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/dxlib_libs/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dxlib/include
        $ENV{DXLIB_PATH}
)

find_library(DXLIB_LIBRARY
    NAMES dxlib
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/dxlib_libs/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dxlib/lib
        $ENV{DXLIB_PATH}
)

# DXLib が見つかった場合、ライブラリをリンク
if(DXLIB_INCLUDE_DIR AND DXLIB_LIBRARY)
    target_include_directories(cubo_othello PRIVATE "${DXLIB_INCLUDE_DIR}")
    target_link_libraries(cubo_othello PRIVATE ${DXLIB_LIBRARY})
else()
    message(WARNING "DXLib library not found. Please set DXLIB_PATH environment variable or provide the correct paths.")
endif()

# ============================================
# ソースファイルの追加（全体設計.txt に従う）
# ============================================
add_executable(cubo_othello
    src/main.cpp              # WinMain + メインループ
    src/board.hpp             # CubeBoard クラスの宣言
    src/game.hpp              # GameEngine クラスの宣言（全体設計より）
    src/ai_player.hpp         # SimpleAI クラスの宣言（全体設計より）
)

# include ディレクトリ：親ディレクトリも検索パスに追加
target_include_directories(cubo_othello PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}       # 親ディレクトリ（board.hpp 等を探すため）
    ${CMAKE_CURRENT_SOURCE_DIR}/src   # src ディレクトリ
)

# ============================================
# オプション：テストビルド（BUILD_TESTS）
# ============================================
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

# ============================================
# メタデータ（プロジェクト情報）
# ============================================
set_property(TARGET cubo_othello APPEND PROPERTY CXX_STANDARD_REQUIRED ON)
set_target_properties(cubo_othello PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)
