cmake_minimum_required(VERSION 3.20)
project(RoN-ESP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RoN-ESP
    src/main.cpp
    src/memory.cpp
    src/game.cpp
    src/overlay.cpp
    src/w2s.cpp
    src/esp.cpp
)

target_include_directories(RoN-ESP PRIVATE src)

# Windows SDK + DirectX
target_link_libraries(RoN-ESP PRIVATE
    d3d11
    d2d1
    dxgi
    dxguid
    dwrite
    User32
    Gdi32
)

# Dear ImGui - download via FetchContent
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
)
set(IMGUI_SOURCES
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui.cpp
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_draw.cpp
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_tables.cpp
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_widgets.cpp
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/backends/imgui_impl_win32.cpp
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/backends/imgui_impl_dx11.cpp
)
FetchContent_MakeAvailable(imgui)

target_sources(RoN-ESP PRIVATE ${IMGUI_SOURCES})
target_include_directories(RoN-ESP PRIVATE
    ${CMAKE_BINARY_DIR}/_deps/imgui-src
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/backends
)
