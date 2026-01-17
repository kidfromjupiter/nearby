FROM ubuntu:24.04

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

RUN apt-get install -y pkg-config libasound2-dev

RUN apt-get install -y libunistring-dev libldap-dev libkrb5-dev libgpg-error-dev gdb libc6-dbg googletest

#gtest
RUN cd /usr/src/googletest
RUN cmake -S . -B build
RUN cmake --build build -j
RUN cmake --install build

# bash history
RUN SNIPPET="export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" \
    && echo "$SNIPPET" >> "/root/.bashrc"

RUN apt-get install -y libunistring-dev libldap-dev libkrb5-dev libgpg-error-dev ca-certificates tzdata curl && update-ca-certificates && date

WORKDIR /workspace


ENTRYPOINT ["/bin/bash"]
# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>
