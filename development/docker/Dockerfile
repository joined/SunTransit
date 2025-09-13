# joined/sdl2 image used for simulator CI builds

FROM ubuntu:22.04

RUN apt-get update -y -qq && \
    apt-get install -y libsdl2-dev python3-pip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
