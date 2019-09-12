cmake_minimum_required (VERSION 3.15)
#cmake_policy(SET CMP0091 NEW)
project (Viewer LANGUAGES CXX CUDA)

find_package(Vulkan REQUIRED)
find_package(CUDA 10.1 REQUIRED)

add_executable(Viewer main.cpp)

#set_property(TARGET Viewer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET Viewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

option(HEADLESS "Headless Renderer" OFF)
if(HEADLESS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHEADLESS")
  target_link_libraries(Viewer NvPipe)
endif(HEADLESS)

set_target_properties(Viewer PROPERTIES CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_DEPTH_ZERO_TO_ONE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_LEFT_HANDED")

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
endif(WIN32)

include_directories(${PROJECT_SOURCE_DIR}/../glm)
include_directories(${PROJECT_SOURCE_DIR}/../gli)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})

target_link_libraries(Viewer ${Vulkan_LIBRARIES})
target_link_libraries(Viewer $ENV{VULKAN_SDK}/Lib/shaderc_shared.lib)
target_link_libraries(Viewer ${CUDA_LIBRARIES})
