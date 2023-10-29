FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get -y install --no-install-suggests --no-install-recommends \
        build-essential ca-certificates clang bison flex \
        libreadline-dev gawk tcl-dev libffi-dev git \
        graphviz xdot pkg-config python3 libboost-system-dev \
        libboost-python-dev libboost-filesystem-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Yosys
RUN git clone https://github.com/YosysHQ/yosys && \
    cd yosys && \
    git checkout 6e907ac && \
    make config-clang && \
    make -j $(nproc) && \
    make install && \
    rm -rf yosys

# Create an user
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
RUN groupadd -o --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME}

COPY --chown=user:user annotation/ /pass/annotation/
COPY --chown=user:user method/ /pass/method/
COPY --chown=user:user include/ /pass/include/
COPY --chown=user:user Makefile /pass/
RUN chown user:user /pass

WORKDIR /pass
USER user:user

RUN [ "/bin/bash", "-c", "make METHOD=surgefuzz" ]
