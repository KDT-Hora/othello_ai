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
    src/board.hpp   # ヘッダにクラス定義を含むため、cpp ファイルは不要（ヘッダのみ includes）
)

target_include_directories(cubo_othello PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${DXLIB_PATH}/include
)

# DxLib ライブラリのリンク（Windows 用）
if(WIN32)
    find_library(DXLIB_LIB dxlib PATHS "${DXLIB_PATH}/lib" NO_DEFAULT_PATH)
    if(NOT DXLIB_LIB)
        # DxLib が見つからない場合は、簡易コンパイルモードでリンクしない
        message(WARNING "DxLib library not found. Building without linking dxlib.lib")
    else()
        target_link_libraries(cubo_othello PRIVATE ${DXLIB_LIB})
    endif()
endif()

# ビルドオプション
option(BUILD_TESTS "ビルド時にテストを含める" OFF)
option(DEBUG_BUILD "デバッグビルド（未最適化）" ON)

if(DEBUG_BUILD)
    add_compile_definitions(CUBE_OTHELLO_DEBUG)
else()
    if(MSVC)
        add_compile_options(/O2 /Ob1 /DNDEBUG)
    elseif(MINGW)
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# ビルドメッセージ
message(STATUS "=== Cube Othello Build Configuration ===")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  DxLib Path:   ${DXLIB_PATH}")
message(STATUS "  Debug Build:  ${DEBUG_BUILD}")
message(STATUS "  Windows:      ${WIN32}")

# インストール設定
install(TARGETS cubo_othello RUNTIME DESTINATION bin)
