FROM ubuntu:20.04
LABEL maintainer="luncliff@gmail.com"

RUN apt update -y -qq &&\
    apt install -y -qq g++-10 rsync tar unzip wget cmake python3

ENV CC=gcc-10 CXX=g++-10
WORKDIR /code

COPY . .
RUN bash ./scripts/install-libcxx.sh
RUN cmake . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_SHARED_LIBS=True \
    cmake --build . --config debug --target install

# Aome test codes lead to GCC crash. Allow failure for now ...
RUN cmake . \
    -DBUILD_TESTING=True &&\
    cmake --build . --config debug | true
