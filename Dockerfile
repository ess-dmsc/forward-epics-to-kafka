FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

ARG http_proxy

ARG https_proxy

RUN apt-get update -y && \
    apt-get --no-install-recommends -y install make cmake g++ git python-pip tzdata vim-common && \
    apt-get -y autoremove && \
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/*

RUN pip install --upgrade pip==9.0.3 && pip install setuptools && \
    pip install conan==1.8.2 && \
    rm -rf /root/.cache/pip/*

# Force conan to create .conan directory and profile
RUN conan profile new default

# Replace the default profile and remotes with the ones from our Ubuntu build node
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/master/files/registry.txt" "/root/.conan/registry.txt"
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/master/files/default_profile" "/root/.conan/profiles/default"

RUN mkdir forwarder

COPY conan/ ../forwarder_src/conan/
RUN cd forwarder && conan install --build=outdated ../forwarder_src/conan/conanfile.txt
COPY cmake/ ../forwarder_src/cmake/
COPY CMakeLists.txt ../forwarder_src
COPY src/ ../forwarder_src/src

RUN cd forwarder && \
    cmake -DCONAN="MANUAL" ../forwarder_src && \
    make -j4 forward-epics-to-kafka VERBOSE=1

ADD docker_launch.sh /
CMD ["./docker_launch.sh"]
