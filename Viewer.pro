SOURCES = main.cpp

DEFINES += VK_USE_PLATFORM_XCB_KHR
DEFINES += _DEBUG

CONFIG += debug c++17

#LIBS += -L"C:/VulkanSDK/1.1.85.0/Lib" -lvulkan-1
LIBS += -lvulkan -lstdc++fs

QT += widgets
QT += x11extras
