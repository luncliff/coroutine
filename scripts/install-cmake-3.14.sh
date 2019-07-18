#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Install CMake 3.14                                                   "
echo "  - Version   : 3.14.5                                                "
echo "  - Source    : GitHub Release                                        "
echo "                                                                      "
echo "----------------------------------------------------------------------"

wget -q https://github.com/Kitware/CMake/releases/download/v3.14.5/cmake-3.14.5-Linux-x86_64.tar.gz
tar -xf cmake-3.14.5-Linux-x86_64.tar.gz
mv ./cmake-3.14.5-Linux-x86_64 ./cmake-3.14.5

rsync -ah ./cmake-3.14.5/bin     /usr
rsync -ah ./cmake-3.14.5/share   /usr

cmake --version
