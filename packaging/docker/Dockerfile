# Multi-stage Dockerfile for FPGA NPU
# Build stage for creating development and runtime images

# Build stage
FROM ubuntu:20.04 AS builder

# Set non-interactive frontend for apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    linux-headers-generic \
    pkg-config \
    libsystemd-dev \
    dkms \
    python3 \
    python3-pip \
    doxygen \
    sphinx-build \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Create build user
RUN useradd -m -s /bin/bash builder && \
    echo "builder ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Set working directory
WORKDIR /workspace

# Copy source code
COPY --chown=builder:builder . .

# Switch to build user
USER builder

# Build the project
RUN mkdir -p build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DENABLE_TESTING=ON \
        -DENABLE_DOCUMENTATION=ON && \
    make -j$(nproc) && \
    make test && \
    make docs

# Install to staging directory
RUN cd build && \
    make install DESTDIR=/workspace/staging

# Runtime stage
FROM ubuntu:20.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    linux-image-generic \
    kmod \
    udev \
    systemd \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create npuuser for running applications
RUN useradd -m -s /bin/bash npuuser && \
    usermod -aG users npuuser

# Copy built artifacts from builder stage
COPY --from=builder /workspace/staging /

# Copy configuration files
COPY --from=builder /workspace/packaging/systemd/fpga-npu.service /lib/systemd/system/
COPY --from=builder /workspace/packaging/udev/99-fpga-npu.rules /lib/udev/rules.d/
COPY --from=builder /workspace/packaging/modprobe/fpga-npu.conf /etc/modprobe.d/

# Update library cache
RUN ldconfig

# Create necessary directories
RUN mkdir -p /var/log/fpga-npu /etc/fpga-npu

# Set permissions
RUN chmod 755 /usr/local/bin/npu-* && \
    chown root:users /dev || true

# Enable systemd service
RUN systemctl enable fpga-npu.service || true

# Expose any necessary ports (if daemon has network interface)
EXPOSE 8080

# Set up entrypoint
COPY scripts/docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["npu-daemon"]

# Development stage
FROM builder AS devel

USER root

# Install additional development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    strace \
    perf-tools-unstable \
    clang-format \
    clang-tidy \
    cppcheck \
    vim \
    emacs \
    && rm -rf /var/lib/apt/lists/*

# Copy development files
COPY --from=builder /workspace/staging /

# Install development packages
RUN cd /workspace/build && make install

# Set up development environment
ENV NPU_DEBUG=1
ENV NPU_LOG_LEVEL=DEBUG

# Switch back to builder user
USER builder

WORKDIR /workspace

CMD ["/bin/bash"]

# Testing stage
FROM devel AS testing

USER root

# Install testing dependencies
RUN apt-get update && apt-get install -y \
    python3-pytest \
    python3-numpy \
    python3-matplotlib \
    lcov \
    gcovr \
    && rm -rf /var/lib/apt/lists/*

USER builder

# Run comprehensive tests
RUN cd build && \
    make test && \
    make coverage || true

# Documentation stage  
FROM nginx:alpine AS docs

# Copy documentation from builder
COPY --from=builder /workspace/build/docs/html /usr/share/nginx/html

# Copy custom nginx config
COPY scripts/nginx-docs.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]

# Package stage for creating distribution packages
FROM ubuntu:20.04 AS packager

ENV DEBIAN_FRONTEND=noninteractive

# Install packaging tools
RUN apt-get update && apt-get install -y \
    build-essential \
    devscripts \
    debhelper \
    dh-systemd \
    rpm \
    rpmbuild \
    fakeroot \
    alien \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /packaging

# Copy source and packaging files
COPY --from=builder /workspace /src
COPY packaging/ /packaging/

# Build packages
RUN cd /src && \
    tar -czf /packaging/fpga-npu_1.0.0.orig.tar.gz \
        --exclude='.git' \
        --exclude='build' \
        --exclude='dist' \
        . && \
    cp -r packaging/debian /src/ && \
    cd /src && \
    dpkg-buildpackage -us -uc

# Build RPM packages
RUN mkdir -p /root/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS} && \
    cp /packaging/fpga-npu_1.0.0.orig.tar.gz /root/rpmbuild/SOURCES/ && \
    cp packaging/rpm/fpga-npu.spec /root/rpmbuild/SPECS/ && \
    rpmbuild -ba /root/rpmbuild/SPECS/fpga-npu.spec

# Create final package collection
FROM scratch AS packages

COPY --from=packager /packaging/*.deb /packages/debian/
COPY --from=packager /root/rpmbuild/RPMS/*/*.rpm /packages/rpm/
COPY --from=packager /root/rpmbuild/SRPMS/*.rpm /packages/rpm/