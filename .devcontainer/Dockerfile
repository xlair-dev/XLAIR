ARG SIV3D_IMAGE=ghcr.io/xlair-dev/siv3d-docker-base:latest
FROM ${SIV3D_IMAGE}

USER root

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libasound2-plugins \
        pulseaudio-utils \
    && rm -rf /var/lib/apt/lists/*

COPY .devcontainer/asound.conf /etc/asound.conf

USER user
