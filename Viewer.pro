SOURCES = main.cpp

#DEFINES += VK_USE_PLATFORM_XCB_KHR
DEFINES += VK_USE_PLATFORM_WIN32_KHR
DEFINES += _DEBUG

CONFIG += debug 
CONFIG += console
CONFIG += c++17


LIBS += -L"D:/VulkanSDK/1.1.114.0/Lib" -lvulkan-1
#LIBS += -lvulkan -lstdc++fs

QT += widgets
#QT += x11extras
