#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
verion_name=$(. /etc/os-release;echo $VERSION)
verion_id=$(. /etc/os-release;echo $VERSION_ID)
distribution=$(. /etc/os-release;echo $UBUNTU_CODENAME)

echo ""
echo "os-release: ${verion_name}"
echo "compilers:"
echo " - g++-8"
echo " - g++-9"
echo " - clang-8"
echo " - clang-9"
echo " - clang-10 (18.04+)"
echo ""

# display release info. this will helpful for checking CI environment
cat /etc/os-release

apt update -qq
apt install -y -qq \
  build-essential software-properties-common
apt-add-repository -y ppa:ubuntu-toolchain-r/test

# http://apt.llvm.org/
# This library will use clang up to 'qualification branch'
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-add-repository "deb http://apt.llvm.org/${distribution}/ llvm-toolchain-${distribution}-8 main"
apt-add-repository "deb http://apt.llvm.org/${distribution}/ llvm-toolchain-${distribution}-9 main"
if [[ ${version_id} == "18.04" ]]; then
  apt-add-repository "deb http://apt.llvm.org/${distribution}/ llvm-toolchain-${distribution}-10 main";
fi

apt update -qq
apt install -y -q \
  g++-8 g++-9 clang-8 clang-9

if [[ ${version_id} == "18.04" ]]; then
  apt install -y -q clang-10;
fi

pushd /usr/bin
ln -s --force gcc-9 gcc
ln -s --force g++-9 g++
ln -s --force clang-9 clang
popd
