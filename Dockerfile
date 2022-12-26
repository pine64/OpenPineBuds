FROM rust:1-alpine3.16 as programmer_build
LABEL maintainer="Ben V. Brown <ralim@ralimtek.com>"
WORKDIR /usr/src
RUN apk add --no-cache git musl-dev
RUN git clone https://github.com/Ralim/bestool.git
RUN cd /usr/src/bestool/bestool/ && cargo build --release

FROM alpine:3.16
LABEL maintainer="Ben V. Brown <ralim@ralimtek.com>"

COPY --from=programmer_build /usr/src/bestool/bestool/target/release/bestool /usr/local/bin/bestool

RUN apk add --no-cache gcc-arm-none-eabi newlib-arm-none-eabi make git bash
WORKDIR /src
# Git trust
RUN git config --global --add safe.directory /src

COPY . /src
