#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
wget -q https://github.com/Kitware/CMake/releases/download/v3.14.0/cmake-3.14.0-Linux-x86_64.tar.gz
tar -xf cmake-3.14.0-Linux-x86_64.tar.gz

rsync -ah ./cmake-3.14.0-Linux-x86_64/bin   /usr
rsync -ah ./cmake-3.14.0-Linux-x86_64/share /usr

cmake --version
