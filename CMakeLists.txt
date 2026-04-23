cmake_minimum_required(VERSION 3.20)
project(MapCompletionTracker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(MapCompletionTracker SHARED
    src/AddonMain.cpp
    src/Shared.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
)

target_include_directories(MapCompletionTracker PRIVATE
    src
    third_party
    third_party/imgui
)

target_link_libraries(MapCompletionTracker PRIVATE
    d3d11
    dxgi
    winhttp
)

set_target_properties(MapCompletionTracker PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
)

target_compile_options(MapCompletionTracker PRIVATE -Wall -Wno-unused-parameter)

target_link_options(MapCompletionTracker PRIVATE
    -static-libgcc
    -static-libstdc++
    -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic
)
