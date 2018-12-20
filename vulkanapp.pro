
HEADERS = \
Viewer.h \
Innovator/Nodes.h \
Innovator/Camera.h \
Innovator/RenderManager.h \
Innovator/Core/Node.h \
Innovator/Core/File.h \
Innovator/Core/State.h \
Innovator/Core/VulkanObjects.h \
Innovator/Core/Misc/Defines.h \
Innovator/Core/Misc/Factory.h \
Innovator/Core/Math/Vector.h \
Innovator/Core/Math/Matrix.h \
Innovator/Core/Math/Octree.h \
Innovator/Core/Scheme/Scheme.h \
Innovator/Core/Vulkan/Wrapper.h

SOURCES = \
main.cpp

TEMPLATE = vcapp

DEFINES += VK_USE_PLATFORM_WIN32_KHR

CONFIG += debug c++1z

QT += widgets
