#!/usr/bin/env bash

# CMake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ bionic main"
sudo apt-get update
sudo apt-get install -y cmake
