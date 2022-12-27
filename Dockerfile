FROM rust:1-slim-buster as programmer_build
LABEL maintainer="Ben V. Brown <ralim@ralimtek.com>"
WORKDIR /usr/src
RUN apt-get update && apt-get install -y git pkg-config libudev-dev bc
RUN git clone https://github.com/Ralim/bestool.git
RUN cd /usr/src/bestool/bestool/ && cargo build --release

FROM debian:buster
LABEL maintainer="Ben V. Brown <ralim@ralimtek.com>"


RUN apt update && apt-get install -y make git bash curl tar bzip2 bc

WORKDIR /src
# Git trust
RUN git config --global --add safe.directory /src
# Grab arm compiler; we have to use this ancient one or else we get boot failures. Probably subtle link issues.

RUN curl https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2 | tar -xj
ENV PATH="${PATH}:/src/gcc-arm-none-eabi-9-2019-q4-major/bin"
WORKDIR /usr/src
COPY --from=programmer_build /usr/src/bestool/bestool/target/release/bestool /usr/local/bin/bestool
COPY . /usr/src
