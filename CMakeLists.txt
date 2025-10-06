cmake_minimum_required(VERSION 3.30)
project(Voxploria LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(PROJ_OUT "${CMAKE_SOURCE_DIR}/out")

add_executable(Voxploria src/main.cpp)

set_target_properties(Voxploria PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJ_OUT}/bin/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${PROJ_OUT}/bin/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${PROJ_OUT}/lib/$<CONFIG>"
)

if (MSVC)
  target_compile_options(Voxploria PRIVATE /W4 /permissive- /Zc:preprocessor /EHsc)
endif()

target_link_libraries(Voxploria PRIVATE Vulkan::Vulkan glfw glm::glm)

if (MSVC)
  target_link_options(Voxploria PRIVATE
    $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup>
  )
endif()
