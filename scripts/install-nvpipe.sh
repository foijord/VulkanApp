#!/usr/bin/env bash

# NvPipe
git clone https://github.com/NVIDIA/NvPipe.git
pushd NvPipe
cmake -DNVPIPE_WITH_DECODER=OFF -DNVPIPE_WITH_OPENGL=OFF -DNVPIPE_BUILD_EXAMPLES=OFF .
sudo make install
popd
