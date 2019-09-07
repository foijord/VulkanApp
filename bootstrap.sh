#!/usr/bin/env bash

# GCP Compute VM
gcloud beta compute --project=foijord-project instances create instance-1 \
--zone=europe-west1-d \
--machine-type=n1-standard-4 \
--subnet=default \
--network-tier=PREMIUM \
--metadata=ssh-keys=foijord:ssh-rsa\ AAAAB3NzaC1yc2EAAAADAQABAAACAQDMv9ncQcD0RZoP5Pl1j3loq/6Bfd8xCzFvmU12sURMBSgQw871dknfjf4RfWebN1tXRkQ5doG\+AJG1pEBehYMFhxKSIu5bsw\+JzJIrDKJ1arWGMsC4l9o6Xk\+IGV62DVWGcuGMONgiPAzJBMzJ4qytY6zGYzOOEUoisoTna2cKIk\+of9teVz13jKyZ4dpXoYPNocDDMy0ttSxolufKOzpDbveTHr2oHbuYsoytA4tNdBV8F998TFp\+0lepKo43RW1NOC4qjNhpbbp2GWJNnMDlS4/IfyJI5mEYlmj4yvI0570ybUJUn1co\+po7I4zaAjIf8F\+GkqhK7//Aenly78YyuuTO2iVHqqW6WXth41q8RIy0VfPQR0jp2o3bhY1bHJNLTQraeSTNRfl9AF5LNlkw7BHErSmmE\+dxpwjPARZqUaRFc3NuZD5UyuZwOgCgfsqS0JxRyd8h0\+R\+gH9WURuGdq\+ES\+V6EqnxhJit9Xsf8IIClFJJps2ej/EGkB3jCz6yAz9vuP2XQHf9aw6Ii/lZIPme17aMSL2xmmY1wZvDCpUiu99qhBQ24/x9gKJ3zr7xORXoT99TwKt7l4LjQr8Acqb43MCzr7W2p2jsthHSgYtMsOkXm\+rI7\+TJVBhuuUAGmj/cqhzzAm0piFg\+T8iANikKFgY/ysD\+pGmAIWTiOQ==\ foijord@DESKTOP-1MG0B62 \
--maintenance-policy=TERMINATE \
--service-account=563567856031-compute@developer.gserviceaccount.com \
--scopes=https://www.googleapis.com/auth/devstorage.read_only,https://www.googleapis.com/auth/logging.write,https://www.googleapis.com/auth/monitoring.write,https://www.googleapis.com/auth/servicecontrol,https://www.googleapis.com/auth/service.management.readonly,https://www.googleapis.com/auth/trace.append \
--accelerator=type=nvidia-tesla-k80,count=1 \
--image=ubuntu-1804-bionic-v20190813a \
--image-project=ubuntu-os-cloud \
--boot-disk-size=256GB \
--boot-disk-type=pd-ssd \
--boot-disk-device-name=instance-1 \
--reservation-affinity=any


# NVIDIA driver
sudo add-apt-repository -y ppa:graphics-drivers
sudo apt-get update
sudo apt-get install -y nvidia-driver-435

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

git clone https://github.com/g-truc/gli.git
git clone https://github.com/g-truc/glm.git
