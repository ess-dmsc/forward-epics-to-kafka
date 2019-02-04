FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

ARG http_proxy

ARG https_proxy

ARG local_conan_server

RUN apt-get update -y && \
    apt-get --no-install-recommends -y install build-essential git python-pip cmake tzdata vim  && \
    apt-get -y autoremove && \
    apt-get clean all && \
    rm -rf /var/lib/apt/lists/*

RUN pip install --upgrade pip==9.0.3 && pip install setuptools && \
    pip install conan && \
    rm -rf /root/.cache/pip/*

# Force conan to create .conan directory and profile
RUN conan profile new default

# Replace the default profile and remotes with the ones from our Ubuntu build node
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/master/files/registry.json" "/root/.conan/registry.json"
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/master/files/default_profile" "/root/.conan/profiles/default"

# Add local Conan server
RUN if [ ! -z "$local_conan_server" ]; then conan remote add --insert 0 ess-dmsc-local "$local_conan_server"; fi

RUN mkdir forwarder

COPY conan/ ../forwarder_src/conan/
RUN cd forwarder && conan install --build=outdated ../forwarder_src/conan/conanfile.txt
COPY cmake/ ../forwarder_src/cmake/
COPY CMakeLists.txt ../forwarder_src
COPY src/ ../forwarder_src/src

RUN cd forwarder && \
    cmake -DCONAN="MANUAL" --target=forward-epics-to-kafka ../forwarder_src && \
    make -j4 forward-epics-to-kafka VERBOSE=1

ADD docker_launch.sh /
CMD ["./docker_launch.sh"]
