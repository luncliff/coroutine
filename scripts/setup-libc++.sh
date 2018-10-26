#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
echo "This script will do..."
echo ""
echo " - Download llvm, libcxx, libcxxabi (version 6.0)"
echo " - Build libcxx/libcxxabi"
echo " - Tree build results"
echo "" 

# https://stackoverflow.com/questions/5947742/how-to-change-the-output-color-of-echo-in-linux
RED='\033[0;31m'
NC='\033[0m' # No Color

# get the packages with specified version
if [ ! -f llvm.zip ]; then
    wget -q -O llvm.zip https://github.com/llvm-mirror/llvm/archive/release_60.zip &
fi
if [ ! -f libcxx.zip ]; then
    wget -q -O libcxx.zip https://github.com/llvm-mirror/libcxx/archive/release_60.zip &
fi
if [ ! -f libcxxabi.zip ]; then
    wget -q -O libcxxabi.zip https://github.com/llvm-mirror/libcxxabi/archive/release_60.zip &
fi
# wait for background downloads
for pid in `jobs -p`
do
    wait $pid
done
rm -rf wget*
echo "${RED}Download done.${NC} "

# !!! Parallel unzip !!!
# parallel --version
# time echo "unzip -q llvm.zip; unzip -q libcxx.zip; unzip -q libcxxabi.zip" | parallel

# unzip and rename directories
if [ ! -d llvm ]; then
   unzip -q llvm.zip;
    mv ./llvm-release_60 ./llvm;
fi
if [ ! -d libcxx ]; then
    unzip -q libcxx.zip;
    mv ./libcxx-release_60 ./libcxx;
fi
if [ ! -d libcxxabi ]; then
    unzip -q libcxxabi.zip;
    mv ./libcxxabi-release_60 ./libcxxabi;
fi
echo "${RED}Unzip done.${NC} "

# build libcxx
mkdir -p prebuilt && pushd prebuilt;
    cmake ../libcxx -DLLVM_PATH=../llvm -DLIBCXX_CXX_ABI=libcxxabi -DLIBCXX_CXX_ABI_INCLUDE_PATHS=../libcxxabi/include -DCMAKE_INSTALL_PREFIX=. -DCMAKE_BUILD_TYPE=Release ;
    make -j7 install;
    rm CMakeCache.txt;
popd;
# build libcxxabi
mkdir -p prebuilt && pushd prebuilt;
    cmake ../libcxxabi -DLLVM_PATH=../llvm/ -DLIBCXXABI_LIBCXX_PATH=../libcxx/ -DCMAKE_INSTALL_PREFIX=../prebuilt/ ;
    make -j7 install;
    rm CMakeCache.txt;
popd;
echo "${RED}Build done.${NC} "

# display build results
tree ./prebuilt/include;
tree ./prebuilt/lib;
