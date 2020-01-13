#!/bin/bash
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
branch=${1:-"master"}
install_prefix=$(realpath ${2:-"/usr"})

echo "----------------------------------------------------------------------"
echo "                                                                      "
echo " Build/Install LLVM project modules                                   "
echo "  - Branch   : ${branch}                                              "
echo "  - Path     : ${install_prefix}/{include, lib}                       "
echo "                                                                      "
echo "----------------------------------------------------------------------"

provider="https://github.com/llvm-mirror"

# get the packages with specified version
for repo in "llvm" "libcxx" "libcxxabi" "clang"
do
    uri="${provider}/${repo}/archive/${branch}.zip"
    wget -q -O "/tmp/${repo}.zip" ${uri} &
done
# wait for background downloads
for pid in $(jobs -p); do
    wait $pid
done

pushd /tmp
for repo in "llvm" "libcxx" "libcxxabi" "clang"
do
    unzip -u -q "${repo}.zip"
    mv "${repo}-${branch}" "${repo}"
done

mkdir -p llvm-build && pushd llvm-build
cmake -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;clang" \
      -DCMAKE_INSTALL_PREFIX="${install_prefix}" \
      ../llvm

# too many logs. make silent ...
make -j4 install-clang  2>err-clang.txt
make -j4 install-cxx    2>err-libcxx.txt
make -j4 install-cxxabi 2>err-libcxxabi.txt
popd
