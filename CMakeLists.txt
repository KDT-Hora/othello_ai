cmake_minimum_required(VERSION 3.15)
project(cubo_othello VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ビルドタイプ（Debug / Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# コンパイラ設定
include(CMakePackageConfigHelpers)

# オプション：デバッグフラグの有効化
option(BUILD_TESTS "Build unit tests" OFF)
option(DEBUG_FLAGS "Enable debug flags (-Wall -Wextra)" OFF)

if(DEBUG_FLAGS)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(cubo_othello
    src/board.hpp
    src/game.hpp
    src/ai_player.hpp
    src/main.cpp
)

# include ディレクトリ設定：親ディレクトリも検索パスに追加（相対パス対策）
target_include_directories(cubo_othello PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}       # 親ディレクトリ（board.hpp 等を探すため）
    ${CMAKE_CURRENT_SOURCE_DIR}/src   # src ディレクトリ
)

# ビルドオプション
if(BUILD_TESTS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

# インストール設定（省略）
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
