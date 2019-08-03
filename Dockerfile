#
#   Author: github.com/luncliff (luncliff@gmail.com)
#
#   The purpose of this Dockerfile is to build GCC for C++ 20 Coroutines
#   https://cloud.docker.com/repository/docker/luncliff/gcc
#

# basic setup with latest ubuntu
FROM ubuntu as build
LABEL maintainer="luncliff@gmail.com"

# prepare tools for build
RUN apt update -qq && apt install -y -qq --fix-missing --no-install-recommends \
    build-essential pkg-config \
    ca-certificates software-properties-common \
    subversion gzip bzip2 \
    g++-7 curl tree m4 flex

# prepare the gcc's source code (coroutine branch)
RUN svn checkout --quiet svn://gcc.gnu.org/svn/gcc/branches/c++-coroutines /gcc

# install prerequisites. of course, the script will do it
WORKDIR /gcc
RUN ./contrib/download_prerequisites

# move to build directory + configure
# see also
#   - https://github.com/namhyung/uftrace/wiki/uftrace-for-gcc-compiler?fbclid=IwAR1J0mcTRkRbAhg9LC9xUAH1_cE7dLSSGDEXlDeuMVVBtBJsTaPtokHi29k
WORKDIR /gcc-build
RUN /gcc/configure --enable-languages=c,c++ \
    --prefix=/usr/local \
    --program-suffix=-10 \
    --disable-multilib  \
    --disable-bootstrap \
    --with-newlib
RUN make --quiet -j4 > build.log && make install

# check the result. notice the version !
RUN which gcc-10 && gcc-10 --version
