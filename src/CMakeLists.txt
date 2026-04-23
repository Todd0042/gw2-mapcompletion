cmake_minimum_required(VERSION 3.20)
project(MapCompletionTracker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Output as a DLL ----
add_library(MapCompletionTracker SHARED
    src/AddonMain.cpp
    src/Shared.cpp
)

target_include_directories(MapCompletionTracker PRIVATE
    src
    # Path to your local copy of nlohmann/json (single-header json.hpp)
    # Download from: https://github.com/nlohmann/json/releases
    third_party/nlohmann
    # Path to your Dear ImGui headers (same version Nexus uses: v1.80)
    # Clone: https://github.com/RaidcoreGG/imgui-v1.80
    third_party/imgui
    # Windows SDK (should be found automatically by MSVC/CMake)
    # DirectX 11 headers (part of Windows SDK)
)

# Link against DirectX 11 (needed for ID3D11ShaderResourceView in Nexus.h)
target_link_libraries(MapCompletionTracker PRIVATE d3d11)

# Export only GetAddonDef (everything else is imported at runtime from Nexus)
set_target_properties(MapCompletionTracker PROPERTIES
    PREFIX ""               # no "lib" prefix on Windows
    SUFFIX ".dll"
)

# Windows-specific settings
if(MSVC)
    # Match the runtime that Nexus/GW2 uses
    set_property(TARGET MapCompletionTracker PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(MapCompletionTracker PRIVATE /W4 /wd4100)
endif()
