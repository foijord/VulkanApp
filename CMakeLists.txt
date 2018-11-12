# cmake.exe -G "Visual Studio 15 2017 Win64" ../../code/VulkanApp/ -DCMAKE_PREFIX_PATH=D:/Qt/5.10.1/msvc2017_64

cmake_minimum_required(VERSION 3.10)

project(VulkanApp)

set(TARGET_NAME ${PROJECT_NAME})
set(CMAKE_AUTOMOC ON)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

find_package(Vulkan REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${TARGET_NAME}
  main.cpp
  Viewer.h
  Innovator/Nodes.h
  Innovator/Camera.h
  Innovator/RenderManager.h
  Innovator/Core/Node.h
  Innovator/Core/File.h
  Innovator/Core/State.h
  Innovator/Core/VulkanObjects.h
  Innovator/Core/Misc/Defines.h
  Innovator/Core/Math/Vector.h
  Innovator/Core/Math/Matrix.h
  Innovator/Core/Math/Octree.h
  Innovator/Core/Scheme/Scheme.h
  Innovator/Core/Vulkan/Wrapper.h)

target_link_libraries(${TARGET_NAME} Qt5::Core)
target_link_libraries(${TARGET_NAME} Qt5::Widgets)
target_link_libraries(${TARGET_NAME} ${Vulkan_LIBRARY})
target_link_libraries(${TARGET_NAME} "$ENV{VULKAN_SDK}/shaderc/build/libshaderc/Debug/shaderc_sharedd.lib")

set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)

include(ExternalProject)

ExternalProject_Add(
  glm
  PREFIX glm
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  CONFIGURE_COMMAND ""
  URL https://github.com/g-truc/glm/archive/0.9.8.5.tar.gz)

ExternalProject_Add(
  gli
  PREFIX gli
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  CONFIGURE_COMMAND ""
  URL https://github.com/g-truc/gli/archive/0.8.2.0.tar.gz)

add_dependencies(${PROJECT_NAME} glm)
add_dependencies(${PROJECT_NAME} gli)

include_directories(${CMAKE_BINARY_DIR}/glm/src/glm)
include_directories(${CMAKE_BINARY_DIR}/gli/src/gli)

file(GLOB_RECURSE GLSL_SOURCE_FILES
  "Shaders/*.vert"
  "Shaders/*.tesc"
  "Shaders/*.tese"
  "Shaders/*.geom"
  "Shaders/*.frag"
  "Shaders/*.comp")

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:${TARGET_NAME}>/Shaders)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o $<TARGET_FILE_DIR:${TARGET_NAME}>/Shaders/${FILE_NAME}.spv
    DEPENDS ${GLSL})
  
endforeach(GLSL)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Textures $<TARGET_FILE_DIR:${TARGET_NAME}>/Textures)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Scenes $<TARGET_FILE_DIR:${TARGET_NAME}>/Scenes)
