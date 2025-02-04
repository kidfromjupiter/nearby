FROM ubuntu:24.10

  # Install dependencies
RUN apt-get update && apt-get install -y \
  build-essential \
  clang \
  cmake \
  git \
  curl \
  libsdbus-c++-dev \
  libssl-dev \
  libsystemd-dev \
  libcurlpp-dev \
    apt-transport-https \
    gnupg \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Installing bazel
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg && mv bazel-archive-keyring.gpg /usr/share/keyrings \
 && echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list \
 && apt update && apt install -y bazel

RUN apt-get install -y pkg-config

WORKDIR /workspace

ENTRYPOINT ["/bin/bash"]