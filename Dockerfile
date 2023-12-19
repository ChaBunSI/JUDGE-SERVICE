# Build: docker buildx build -t "이름" .
# 워커: CMakeLists.txt의 add_executables()에 worker.cc 추가
# 브로커: CMakeLists.txt의 add_executables()에 broker.cc 추가

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /home/
COPY . /home/JUDGE-SERVICE
WORKDIR /home/JUDGE-SERVICE
RUN mkdir -p testcases
WORKDIR /home/JUDGE-SERVICE/

ARG is_cpp=1

WORKDIR /home/
RUN apt-get update && apt-get install -y \
    python3 \
    build-essential \
    git \
    gdb \
    cmake \
    libcap-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev \
    zlib1g-dev \
    libpulse-dev \
    rapidjson-dev \
    asciidoc \
&& apt-get clean && rm -rf /var/lib/apt/lists/*

# git clone isolate && install it
RUN git clone https://github.com/ioi/isolate/
WORKDIR /home/isolate
RUN make && make install

# build aws-sdk-cpp
WORKDIR /home/
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
RUN mkdir sdk_build
WORKDIR /home/sdk_build
RUN cmake ../aws-sdk-cpp -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_ONLY="sqs;sns"
RUN make && make install

# make the server ready
WORKDIR /home/JUDGE-SERVICE
RUN mkdir build
WORKDIR /home/JUDGE-SERVICE/build
RUN cmake ../ && make
RUN ls -al
# run the server
CMD ["./JUDGE-SERVICE", ${is_cpp}]
