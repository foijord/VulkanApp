#!/usr/bin/env bash

# NVIDIA driver
sudo add-apt-repository -y ppa:graphics-drivers
sudo apt-get update
sudo apt-get install -y nvidia-driver-430
