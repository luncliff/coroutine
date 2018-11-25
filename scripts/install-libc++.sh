#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "--------------------------------------------------"
echo " !!! Installing libcxx/libcxxabi !!!"
echo "  llvm, libcxx, libcxxabi (version 6.0)"
echo "--------------------------------------------------"

# get the packages with specified version
echo "Download"
if [ ! -f llvm.zip ]; then
    echo "  llvm 6.0"
    wget -q -O llvm.zip         https://github.com/llvm-mirror/llvm/archive/release_60.zip &
fi
if [ ! -f libcxx.zip ]; then
    echo "  libcxx 6.0"
    wget -q -O libcxx.zip       https://github.com/llvm-mirror/libcxx/archive/release_60.zip &
fi
if [ ! -f libcxxabi.zip ]; then
    echo "  libcxxabi 6.0"
    wget -q -O libcxxabi.zip    https://github.com/llvm-mirror/libcxxabi/archive/release_60.zip &
fi
# wait for background downloads
for pid in `jobs -p`
do
    wait $pid
done
rm -rf wget*
echo "Download done"

# unzip and rename directories
echo "Unzip"
if [ ! -d llvm ]; then
    echo "  llvm 6.0"
    unzip -q llvm.zip &
fi
if [ ! -d libcxx ]; then
    echo "  libcxx 6.0"
    unzip -q libcxx.zip &
fi
if [ ! -d libcxxabi ]; then
    echo "  libcxxabi 6.0"
    unzip -q libcxxabi.zip &
fi
# wait for background downloads
for pid in `jobs -p`
do
    wait $pid
done

mv ./llvm-release_60        ./llvm;
mv ./libcxx-release_60      ./libcxx;
mv ./libcxxabi-release_60   ./libcxxabi;
# rm -rf llvm.zip libcxx.zip libcxxabi.zip;
echo "Unzip done"

echo "Build/Install"
echo "Installing to /usr/include & /usr/lib (not /usr/local)"
# build libcxx
mkdir -p prebuilt && pushd prebuilt;
    cmake ../libcxx                                         \
        -DLLVM_PATH=../llvm                                 \
        -DLIBCXX_CXX_ABI=libcxxabi                          \
        -DLIBCXX_CXX_ABI_INCLUDE_PATHS=../libcxxabi/include \
        -DCMAKE_BUILD_TYPE=Release                          \
        -DCMAKE_INSTALL_PREFIX=/usr                         \
        ;
    make -j7 install;
    rm CMakeCache.txt;
popd;
# build libcxxabi
mkdir -p prebuilt && pushd prebuilt;
    cmake ../libcxxabi                      \
        -DLLVM_PATH=../llvm                 \
        -DLIBCXXABI_LIBCXX_PATH=../libcxx/  \
        -DCMAKE_INSTALL_PREFIX=/usr         \
        ;
    make -j7 --silent install;
    rm CMakeCache.txt;
popd;
echo "Build/Install done"
