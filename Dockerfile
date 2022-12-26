FROM alpine:3.16
LABEL maintainer="Ben V. Brown <ralim@ralimtek.com>"

RUN apk add --no-cache gcc-arm-none-eabi newlib-arm-none-eabi make git bash
WORKDIR /src
# Git trust
RUN git config --global --add safe.directory /src

COPY . /src
