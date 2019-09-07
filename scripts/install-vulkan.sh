#!/usr/bin/env bash

# Vulkan SDK
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.1.114-bionic.list http://packages.lunarg.com/vulkan/1.1.114/lunarg-vulkan-1.1.114-bionic.list
sudo apt update
sudo apt install -y vulkan-sdk
