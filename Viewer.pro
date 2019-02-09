SOURCES = main.cpp

DEFINES += VK_USE_PLATFORM_XCB_KHR
DEFINES += _DEBUG

CONFIG += debug c++1z

#LIBS += -L"C:/VulkanSDK/1.1.85.0/Lib" -lvulkan-1
LIBS += -lvulkan

QT += widgets
QT += x11extras
