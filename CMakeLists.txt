cmake_minimum_required (VERSION 3.1)
project (Viewer)
add_executable(Viewer main.cpp)

set_target_properties(Viewer PROPERTIES CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_DEPTH_ZERO_TO_ONE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_LEFT_HANDED")

IF(WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSE()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
ENDIF(WIN32)

find_package(Vulkan REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/../glm)
include_directories(${PROJECT_SOURCE_DIR}/../gli)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${Vulkan_INCLUDE_DIRS})

target_link_libraries(Viewer Vulkan::Vulkan)

