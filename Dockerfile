FROM ubuntu:22.04
RUN apt-get update && apt-get install -y git \
    && apt-get install -y wget \
    && apt-get install -y cmake \
    && apt-get install -y gcc \
    && apt-get install -y g++ \
    && apt-get install -y sshpass \
    && apt-get install -y gdb \
    && apt install libjsoncpp-dev \
    && apt install uuid-dev \
    && apt install openssl \
    && apt install libssl-dev \
    && apt install zlib1g-dev

RUN apt-get install -y libboost-all-dev \
    && apt-get install -y libasio-dev \
    && apt-get install -y rapidjson-dev \
    && apt install -y libwebsocketpp-dev

#RUN cd / \
#    && wget https://github.com/bazelbuild/bazelisk/releases/download/v1.15.0/bazelisk-linux-amd64 \
#    && chmod 775 bazelisk-linux-amd64 \
#    && ./bazelisk-linux-amd64 \
#    && rm bazelisk-linux-amd64 \
#    && ln -s /root/.cache/bazelisk/downloads/bazelbuild/bazel-6.0.0-linux-x86_64/bin/bazel /usr/local/bin/bazel