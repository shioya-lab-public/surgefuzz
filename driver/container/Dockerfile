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

COPY --chown=user:user include/ /driver/include/
COPY --chown=user:user test/ /driver/test/
COPY --chown=user:user Makefile /driver
RUN chown user:user /driver

USER user:user
WORKDIR /driver

RUN [ "/bin/bash", "-c", "make test" ]
