FROM ubuntu:17.10

RUN apt-get update && \
    apt-get -y install cmake g++ git python-pip tzdata vim-common && \
    apt-get -y autoremove && \
    apt-get clean all

RUN pip install --upgrade pip && \
    pip install conan==1.0.2 && \
rm -rf /root/.cache/pip/*

# Force conan to create .conan directory and profile
RUN conan profile new default

# Replace the default profile and remotes with the ones from our Ubuntu 17.10 build node
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu17.10-build-node/master/files/registry.txt" "/root/.conan/registry.txt"
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu17.10-build-node/master/files/default_profile" "/root/.conan/profiles/default"

RUN mkdir forwarder
ADD . /forwarder

RUN mkdir build && cd build && cmake ../forwarder

RUN make --directory=./build -j8 VERBOSE=1

CMD ["./forwarder/docker_launch.sh"]
