FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get -y install --no-install-suggests --no-install-recommends \
        build-essential \
    && rm -rf /var/lib/apt/lists/*

# Create an user
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
RUN groupadd -o --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME}

COPY --chown=user:user src/ /fuzzer/src/
COPY --chown=user:user include/ /fuzzer/include/
COPY --chown=user:user Makefile /fuzzer
RUN chown user:user /fuzzer

WORKDIR /fuzzer
USER user:user

RUN [ "/bin/bash", "-c", "make -j$(nproc)" ]
