FROM mcr.microsoft.com/devcontainers/base:bookworm

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -qqy install \
    # Common stuff
    apt-utils \
    build-essential \
    ca-certificates \
    curl \
    dialog \
    git \
    sudo \
    unzip \
    wget \
    # for platformio
    bc \
    build-essential \
    clang \
    curl \
    gcc \
    python3 \
    python3-dev \
    python3-distutils \
    python3-pip \
    srecord \
    xz-utils
# Clean up
RUN apt-get update && \
    apt-get -qqy install python3-venv \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*
ENV DEBIAN_FRONTEND=dialog

# https://docs.platformio.org/en/latest/faq.html#platformio-udev-rules

RUN echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode \
    && chmod 0440 /etc/sudoers.d/vscode

USER vscode
RUN curl -fsSL -o /tmp/get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
RUN python3 /tmp/get-platformio.py
RUN . ~/.platformio/penv/bin/activate \
    && pio pkg install -g -p espressif32 \
    -t espressif/toolchain-xtensa-esp32s3 \
    -t espressif/toolchain-riscv32-esp \
    -t platformio/framework-arduinoespressif32 \
    -t platformio/tool-esptoolpy \
    -t platformio/tool-openocd-esp32 \
    -t platformio/tool-scons