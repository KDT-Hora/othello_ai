cmake_minimum_required(VERSION 3.15)
project(CubeOthello VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# DxLib 依存（Windows 向け）
set(DXLIB_PATH "${CMAKE_SOURCE_DIR}/DxLib")

if(WIN32)
    set(DXLIB_SOURCES ${DXLIB_PATH}/src/src/duilib.cpp
                      ${DXLIB_PATH}/src/src/dxlib.cpp
                      ${DXLIB_PATH}/src/src/font.cpp
                      ${DXLIB_PATH}/src/src/hle.cpp
                      ${DXLIB_PATH}/src/src/input.cpp)
    set(DXLIB_INCLUDE_DIRS "${DXLIB_PATH}/include")
else()
    message(FATAL_ERROR "DxLib は Windows 向けライブラリです。MSVC++ または MinGW-w64 でコンパイルしてください。")
endif()

# メインプロジェクト
add_executable(cubo_othello
    src/main.cpp
    src/board.cpp
    src/game.cpp
    src/ai.cpp
    src/gui.cpp
)

target_include_directories(cubo_othello PRIVATE ${DXLIB_INCLUDE_DIRS})

# DxLib のライブラリをリンク（Windows）
if(WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(cubo_othello PUBLIC dxlib.lib dxerr.lib dinput8.lib dxguid.lib)
endif()

# ビルドオプション
option(BUILD_TESTS "ビルド時にテストを含める" OFF)
option(DEBUG_BUILD "デバッグビルド（未最適化）" ON)

if(DEBUG_BUILD)
    add_compile_definitions(CUBE_OTHELLO_DEBUG)
    if(WIN32 AND MSVC)
        add_compile_options(/Od /Zi /W4)
    elseif(MINGW)
        add_compile_options(-O0 -g -Wall)
    endif()
else()
    if(WIN32 AND MSVC)
        add_compile_options(/O2 /Ob1 /DNDEBUG)
    elseif(MINGW)
        add_compile_options(-O2 -DNDEBUG)
    endif()
endif()

# ドキュメント生成（Doxygen）
option(BUILD_DOCS "Doxygen ドキュメントをビルド" OFF)
if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif()

# インストーラ（Windows Installer）
option(BUILD_WINDOWS_INSTALLER "Windows installer をビルド" OFF)

install(TARGETS cubo_othello RUNTIME DESTINATION bin)
install(FILES ${DXLIB_PATH}/lib/dxlib.lib DESTINATION lib)
