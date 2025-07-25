FROM ubuntu:24.04

RUN apt update && apt install -y \
    build-essential \
    git \
    tree \
    clang-17 \
    cmake \
    wget \
    curl \
    libstdc++-12-dev \
    g++ \
    vim \
    sudo \
    zsh \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/hsutter/cppfront.git && \
    cd cppfront/source && \
    g++ -std=c++20 -o cppfront cppfront.cpp && \
    cp cppfront /usr/local/bin && \
    cp ../include/cpp2util.h /usr/local/include/

RUN echo 'main: () = {'                                >  /tmp/test.cpp2 && \
    echo '    std::cout << "Hello, world!\\n";'        >> /tmp/test.cpp2 && \
    echo '}'                                           >> /tmp/test.cpp2 && \
    cppfront /tmp/test.cpp2 -o /tmp/test.cpp && \
    g++ -std=c++20 -I/usr/local/include /tmp/test.cpp -o /tmp/test && \
    /tmp/test && \
    echo "cppfront installed successfully!"

RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" || true

WORKDIR /workspace
CMD ["/bin/bash"]
