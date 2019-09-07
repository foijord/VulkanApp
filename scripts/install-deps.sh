#!/usr/bin/env bash

# NVIDIA driver
sudo add-apt-repository -y ppa:graphics-drivers
sudo apt-get update
sudo apt-get install -y nvidia-driver-430

# Vulkan SDK
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.1.114-bionic.list http://packages.lunarg.com/vulkan/1.1.114/lunarg-vulkan-1.1.114-bionic.list
sudo apt update
sudo apt install -y vulkan-sdk

# CUDA Toolkit 10.1
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-ubuntu1804.pin
sudo mv cuda-ubuntu1804.pin /etc/apt/preferences.d/cuda-repository-pin-600
sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub
sudo add-apt-repository "deb http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/ /"
sudo apt-get update
sudo apt-get -y install cuda

export CUDACXX=/usr/local/cuda-10.1/bin/nvcc

# CMake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ bionic main"
sudo apt-get update
sudo apt-get install -y cmake

# NvPipe
git clone https://github.com/NVIDIA/NvPipe.git
pushd NvPipe
cmake -DNVPIPE_WITH_DECODER=OFF -DNVPIPE_WITH_OPENGL=OFF -DNVPIPE_BUILD_EXAMPLES=OFF .
sudo make install
popd
export LD_LIBRARY_PATH=/usr/local/lib

git clone https://github.com/g-truc/gli.git
git clone https://github.com/g-truc/glm.git

cd VulkanApp
cmake -DHEADLESS=ON .
make

git config --global user.name "Frode Oijord"
git config --global user.email "foijord@gmail.com"
