cmake_minimum_required(VERSION 3.26)
project(voxploria LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_program(GLSLC glslc REQUIRED)

include_directories(
    ${GLFW_INCLUDE_DIRS}
    external/stb
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

set(VERT_SHADER ${CMAKE_SOURCE_DIR}/shaders/voxel.vert)
set(FRAG_SHADER ${CMAKE_SOURCE_DIR}/shaders/voxel.frag)
set(VERT_SPV   ${CMAKE_BINARY_DIR}/shaders/voxel.vert.spv)
set(FRAG_SPV   ${CMAKE_BINARY_DIR}/shaders/voxel.frag.spv)

add_custom_command(
    OUTPUT ${VERT_SPV}
    COMMAND ${GLSLC} -O ${VERT_SHADER} -o ${VERT_SPV}
    DEPENDS ${VERT_SHADER}
    COMMENT "Compiling voxel.vert → SPIR-V"
)
add_custom_command(
    OUTPUT ${FRAG_SPV}
    COMMAND ${GLSLC} -O ${FRAG_SHADER} -o ${FRAG_SPV}
    DEPENDS ${FRAG_SHADER}
    COMMENT "Compiling voxel.frag → SPIR-V"
)
add_custom_target(compile_shaders DEPENDS ${VERT_SPV} ${FRAG_SPV})

if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
        COMMENT "Copying assets/"
    )
endif()

add_executable(voxploria
    src/main.cpp
    src/App.cpp
    src/Renderer.cpp
    src/VulkanContext.cpp
    src/Camera.cpp
    src/Lighting.cpp
    src/Noise.cpp
    src/WorldGen.cpp
    src/World.cpp
    src/ChunkMesher.cpp
)

add_dependencies(voxploria compile_shaders)
if(TARGET copy_assets)
    add_dependencies(voxploria copy_assets)
endif()

target_compile_definitions(voxploria PRIVATE
    SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
)

target_link_libraries(voxploria
    Vulkan::Vulkan
    ${GLFW_LIBRARIES}
)
target_link_directories(voxploria PRIVATE ${GLFW_LIBRARY_DIRS})
target_compile_options(voxploria PRIVATE ${GLFW_CFLAGS_OTHER})
