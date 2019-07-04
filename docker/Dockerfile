# This file is derived from the original Dockerfile for Zephyr Project's
# Docker image. The original Dockerfile can be found at Zephyr Project's
# github page.
#   Github repository: https://github.com/zephyrproject-rtos/docker-image
#   Commit: c2a7da444d34c598af113168ae3fcc5ffb1c8d0e
#
# Changes:
#	Remove VNC support.
# 	Cloning Zephyr and KNoT used git repositories.
#	Always using CESAR's Zephyr Project git fork.
#	Install Zephyr-KNoT-SDK requirements.
#	Allow user to use the desired folder as the current working directory.
#	Install Segger Jlink drivers from an external file.

FROM ubuntu:18.04

ARG ZSDK_VERSION=0.10.0
ARG GCC_ARM_NAME=gcc-arm-none-eabi-7-2018-q2-update

ENV DEBIAN_FRONTEND noninteractive

RUN dpkg --add-architecture i386 && \
	apt-get -y update && \
	apt-get -y upgrade && \
	apt-get install --no-install-recommends -y \
	gnupg \
	ca-certificates && \
	apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF && \
	echo "deb https://download.mono-project.com/repo/ubuntu stable-bionic main" | tee /etc/apt/sources.list.d/mono-official-stable.list && \
	apt-get -y update && \
	apt-get install --no-install-recommends -y \
	autoconf \
	automake \
	build-essential \
	ccache \
	device-tree-compiler \
	dfu-util \
	doxygen \
	file \
	g++ \
	gcc \
	gcc-multilib \
	gcovr \
	git \
	git-core \
	gperf \
	gtk-sharp2 \
	iproute2 \
	lcov \
	libglib2.0-dev \
	libgtk2.0-0 \
	libpcap-dev \
	libsdl2-dev:i386 \
	libtool \
	locales \
	make \
	mono-complete \
	net-tools \
	ninja-build \
	openbox \
	pkg-config \
	python3-pip \
	python3-ply \
	python3-setuptools \
	python-xdg \
	qemu \
	socat \
	sudo \
	texinfo \
	valgrind \
	wget \
	x11vnc \
	xvfb \
	xz-utils && \
	wget -O dtc.deb http://security.ubuntu.com/ubuntu/pool/main/d/device-tree-compiler/device-tree-compiler_1.4.7-1_amd64.deb && \
	dpkg -i dtc.deb && \
	wget -O renode.deb https://github.com/renode/renode/releases/download/v1.6.2/renode_1.6.2_amd64.deb && \
	apt install -y ./renode.deb && \
	rm dtc.deb renode.deb && \
	rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZSDK_VERSION}/zephyr-sdk-${ZSDK_VERSION}-setup.run" && \
	sh "zephyr-sdk-${ZSDK_VERSION}-setup.run" --quiet -- -d /opt/toolchains/zephyr-sdk-${ZSDK_VERSION} && \
	rm "zephyr-sdk-${ZSDK_VERSION}-setup.run"

RUN wget -q https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/${GCC_ARM_NAME}-linux.tar.bz2  && \
	tar xf ${GCC_ARM_NAME}-linux.tar.bz2 && \
	rm -f ${GCC_ARM_NAME}-linux.tar.bz2 && \
	mv ${GCC_ARM_NAME} /opt/toolchains/${GCC_ARM_NAME}

RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.13.2/cmake-3.13.2-Linux-x86_64.sh && \
	chmod +x cmake-3.13.2-Linux-x86_64.sh && \
	./cmake-3.13.2-Linux-x86_64.sh --skip-license --prefix=/usr/local && \
	rm -f ./cmake-3.13.2-Linux-x86_64.sh


RUN useradd -m -G plugdev user \
	&& echo 'user ALL = NOPASSWD: ALL' > /etc/sudoers.d/user \
	&& chmod 0440 /etc/sudoers.d/user

# Set the locale
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}
ENV GNUARMEMB_TOOLCHAIN_PATH=/opt/toolchains/${GCC_ARM_NAME}
ENV PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig
ENV DISPLAY=:0

# Get KNoT's Zephyr
ENV ZEPHYR_BASE=/home/user/zephyrproject/zephyr
RUN git clone -b zephyr-knot-v1.14.0 --single-branch --depth=1 https://github.com/CESARBR/zephyr.git ${ZEPHYR_BASE}
RUN pip3 install wheel && \
	pip3 install -r ${ZEPHYR_BASE}/scripts/requirements.txt && \
	pip3 install west &&\
	pip3 install sh

# WEST
RUN (cd ${ZEPHYR_BASE}/.. && west init -l ${ZEPHYR_BASE} && west update)

ENV KNOT_BASE=/home/user/knot-sdk

# Download nRF Command Line Tools
RUN wget -qO nrf5_tools.tar.gz https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/nRFCommandLineTools1021Linuxamd64tar.gz

# Extract nRF Command Line Tools
RUN mkdir nrf5_tools
RUN tar -xvzf nrf5_tools.tar.gz -C nrf5_tools

# Install JLink and nRF Command Line Tools
RUN dpkg -i nrf5_tools/JLink_Linux_V644e_x86_64.deb
RUN dpkg -i nrf5_tools/nRF-Command-Line-Tools_10_2_1_Linux-amd64.deb

RUN rm -rf nrf5_tools nrf5_tools.tar.gz

# KNoT Protocol
RUN apt install automake libtool
RUN git clone -b master --single-branch --depth=1 https://github.com/CESARBR/knot-protocol-source.git && \
	(cd knot-protocol-source && \
	./bootstrap-configure && \
	make) && \
	rm -rf knot-protocol-source

# KNoT SDK
RUN git clone -b master --single-branch --depth=1 https://github.com/CESARBR/zephyr-knot-sdk.git ${KNOT_BASE} && \
	pip3 install -r ${KNOT_BASE}/scripts/requirements.txt && \
	rm -f requirements.txt

# Use cli.py as an installed script
RUN ln -s ${KNOT_BASE}/scripts/cli.py /usr/local/bin/knot

# Clone target OpenThread branch
ARG OT_BASE=/home/user/openthread
RUN git clone -b master --single-branch https://github.com/openthread/openthread.git ${OT_BASE} && \
	(cd ${OT_BASE} && \
	git checkout -b knot_hash f9d757a161fea4775d033a1ce88cf7962fe24a93)

RUN knot ot-path ${OT_BASE}

RUN chown -R user:user /home/user

RUN usermod -a -G dialout user

USER user

CMD ["/bin/bash"]

WORKDIR /workdir
VOLUME ["/workdir"]
