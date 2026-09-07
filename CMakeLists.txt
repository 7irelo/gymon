# Gymon — CMake build.
#
# Mirrors premake5.lua, which remains the Visual Studio project generator. This
# exists so the engine builds from a plain terminal on any platform:
#
#   cmake -B build
#   cmake --build build -j
#   ./build/bin/Sandbox
#
# GLFW, glm and Dear ImGui are git submodules. Run
# `git submodule update --init --recursive` before configuring.
cmake_minimum_required(VERSION 3.20)
project(Gymon LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Everything lands in build/bin so the run command is predictable across
# single- and multi-config generators.
foreach(_out RUNTIME LIBRARY ARCHIVE)
    set(CMAKE_${_out}_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set(CMAKE_${_out}_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_BINARY_DIR}/bin")
    endforeach()
endforeach()

set(GY_VENDOR "${CMAKE_CURRENT_SOURCE_DIR}/Gymon/vendor")

foreach(_dep GLFW glm imgui)
    if(NOT EXISTS "${GY_VENDOR}/${_dep}/CMakeLists.txt" AND NOT EXISTS "${GY_VENDOR}/${_dep}/imgui.h")
        message(FATAL_ERROR
            "Gymon/vendor/${_dep} is empty. Run:\n"
            "  git submodule update --init --recursive")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# GLFW: build the library only, and never its docs/tests/examples.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GY_VENDOR}/GLFW" glfw EXCLUDE_FROM_ALL)

add_library(Glad STATIC "${GY_VENDOR}/Glad/src/glad.c")
target_include_directories(Glad PUBLIC "${GY_VENDOR}/Glad/include")

add_library(ImGui STATIC
    "${GY_VENDOR}/imgui/imgui.cpp"
    "${GY_VENDOR}/imgui/imgui_draw.cpp"
    "${GY_VENDOR}/imgui/imgui_tables.cpp"
    "${GY_VENDOR}/imgui/imgui_widgets.cpp"
    "${GY_VENDOR}/imgui/imgui_demo.cpp"
)
target_include_directories(ImGui PUBLIC "${GY_VENDOR}/imgui")

add_library(stb_image STATIC "${GY_VENDOR}/stb_image/stb_image.cpp")
target_include_directories(stb_image PUBLIC "${GY_VENDOR}/stb_image")

# ---------------------------------------------------------------------------
# Gymon — the engine, built as a static library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE GYMON_SOURCES CONFIGURE_DEPENDS
    "Gymon/src/Gymon/*.cpp"
    "Gymon/src/Platform/OpenGL/*.cpp"
)
list(APPEND GYMON_SOURCES
    "Gymon/src/gypch.cpp"
    "${GY_VENDOR}/imgui/backends/imgui_impl_glfw.cpp"
    "${GY_VENDOR}/imgui/backends/imgui_impl_opengl3.cpp"
)

# The window and input layers sit entirely on GLFW, so one implementation
# serves every desktop platform rather than a per-OS copy of the same code.
list(APPEND GYMON_SOURCES
    "Gymon/src/Platform/GLFW/GlfwWindow.cpp"
    "Gymon/src/Platform/GLFW/GlfwInput.cpp"
)

add_library(Gymon STATIC ${GYMON_SOURCES})

target_include_directories(Gymon PUBLIC
    "Gymon/src"
    "${GY_VENDOR}/spdlog/include"
    "${GY_VENDOR}/glm"
    "${GY_VENDOR}/json"
    "${GY_VENDOR}"
)

target_compile_definitions(Gymon PUBLIC
    GLFW_INCLUDE_NONE
    $<$<CONFIG:Debug>:GY_DEBUG>
    $<$<CONFIG:Release>:GY_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:GY_RELEASE>
    $<$<CONFIG:MinSizeRel>:GY_DIST>
)

if(WIN32)
    target_compile_definitions(Gymon PUBLIC GY_PLATFORM_WINDOWS NOMINMAX _CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(Gymon PUBLIC GY_PLATFORM_LINUX)
elseif(APPLE)
    target_compile_definitions(Gymon PUBLIC GY_PLATFORM_MACOS)
endif()

target_link_libraries(Gymon PUBLIC glfw Glad ImGui stb_image)

if(WIN32)
    target_link_libraries(Gymon PUBLIC opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(Gymon PUBLIC OpenGL::GL ${CMAKE_DL_LIBS})
endif()

target_precompile_headers(Gymon PRIVATE "Gymon/src/gypch.h")

# ---------------------------------------------------------------------------
# Sandbox — the example application
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS "Sandbox/src/*.cpp")
add_executable(Sandbox ${SANDBOX_SOURCES})
target_link_libraries(Sandbox PRIVATE Gymon)

# Sandbox loads assets by relative path, so put them next to the binary.
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/assets"
        "$<TARGET_FILE_DIR:Sandbox>/assets"
    COMMENT "Copying Sandbox assets"
)
