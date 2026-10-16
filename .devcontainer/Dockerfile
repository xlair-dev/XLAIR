ARG SIV3D_IMAGE=ghcr.io/xlair-dev/siv3d-docker-base:latest
FROM ${SIV3D_IMAGE}

USER root

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        curl \
        ca-certificates \
        libasound2-plugins \
        pulseaudio-utils \
    && rm -rf /var/lib/apt/lists/*

COPY .devcontainer/asound.conf /etc/asound.conf

USER user

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs -o /tmp/rustup.sh \
    && sh /tmp/rustup.sh -y --profile minimal --default-toolchain nightly \
    && rm /tmp/rustup.sh

ENV PATH="/home/user/.cargo/bin:${PATH}"
