FROM ubuntu:24.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y \
    nano \
    vim \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    make \
    g++ \
    git \
    libboost-graph-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-thread-dev \
    libboost-program-options-dev \
    build-essential\
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy the entire repository
COPY . .

# Build ggg
WORKDIR /workspace/ggg
# RUN mkdir -p build && cd build && \
#     cmake .. && \
#     make

RUN rm -rf build && \
    mkdir -p build && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ALL_SOLVERS=ON -DBUILD_TOOLS=ON && \
    cmake --build build -j$(nproc)

# Return to workspace root
WORKDIR /workspace

# Create virtual environment
RUN python3 -m venv venv

# Activate virtual environment and install egsolver
RUN . venv/bin/activate && \
    cd egsolver && \
    pip install --upgrade pip && \
    pip install .

# Install Python dependencies for plotting
RUN . venv/bin/activate && \
    pip install numpy matplotlib pandas

# Set working directory to experiments
WORKDIR /workspace/experiments

# Set environment variable to activate venv in shell
ENV PATH="/workspace/venv/bin:$PATH"

# Default command
CMD ["/bin/bash"]

