
cmake_minimum_required(VERSION 3.10)

project(VulkanApp)

set(TARGET_NAME ${PROJECT_NAME})

find_package(Vulkan REQUIRED)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/External/include)

add_executable(${TARGET_NAME} WIN32
  main.cpp
  Viewer.h
  Application.h
  Innovator/Nodes.h
  Innovator/Group.h
  Innovator/Actions.h
  Innovator/Commands.h
  Innovator/Core/Node.h
  Innovator/Core/File.h
  Innovator/Core/State.h
  Innovator/Core/VulkanObjects.h
  Innovator/Core/Misc/Defines.h
  Innovator/Core/Scheme/Scheme.h
  Innovator/Core/Vulkan/Wrapper.h)

target_link_libraries(${TARGET_NAME} ${Vulkan_LIBRARY})

set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)

file(GLOB_RECURSE GLSL_SOURCE_FILES
  "Shaders/*.vert"
  "Shaders/*.tesc"
  "Shaders/*.tese"
  "Shaders/*.geom"
  "Shaders/*.frag"
  "Shaders/*.comp")

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/Shaders)

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
