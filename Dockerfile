FROM debian:bullseye-slim AS base

FROM base AS rust_build

LABEL org.opencontainers.image.authors = "Ben V. Brown <ralim@ralimtek.com>, Dom Rodriguez <shymega@shymega.org.uk>"

WORKDIR /usr/src
ENV PATH="/root/.cargo/bin:$PATH"

RUN apt-get update \
    && apt-get install -y \
    bc \
    build-essential \
    curl \
    git  \
    libudev-dev \
    pkg-config \
    && curl https://sh.rustup.rs -sSf | bash -s -- -y \
    && git clone https://github.com/Ralim/bestool.git \
    && cd /usr/src/bestool/bestool/ \
    && cargo build --release

FROM base as dev_env

WORKDIR /usr/src

RUN apt-get update \
    && apt-get install -y \ 
    bash \
    bc \
    bzip2 \
    clang-format \ 
    cmake \
    curl \
    ffmpeg \
    git \
    make \
    minicom \
    sudo \
    tar \
    xxd \
    && git config --global --add safe.directory /usr/src \
    && mkdir -pv /usr/local/toolchains \
    && curl \
      https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-$(arch)-linux.tar.bz2 \
      | tar -xj -C /usr/local/toolchains

ENV PATH="${PATH}:/usr/local/toolchains/gcc-arm-none-eabi-9-2019-q4-major/bin"
COPY --from=rust_build /usr/src/bestool/bestool/target/release/bestool /usr/local/bin/bestool
COPY . /usr/src

ENTRYPOINT ["/bin/bash"]
