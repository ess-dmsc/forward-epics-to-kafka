FROM ubuntu:18.04

ARG http_proxy

ARG https_proxy

ARG local_conan_server

ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/master/files/registry.json" "/root/.conan/registry.json"
ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/master/files/default_profile" "/root/.conan/profiles/default"

COPY conan/ ../forwarder_src/conan/

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update -y \
    && apt-get --no-install-recommends -y install build-essential git python-pip cmake tzdata vim \
    && apt-get -y autoremove \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    && pip install --upgrade pip==9.0.3 \
    && pip install setuptools \
    && pip install conan \
    && rm -rf /root/.cache/pip/* \
    && mkdir forwarder \
    && if [ ! -z "$local_conan_server" ]; then conan remote add --insert 0 ess-dmsc-local "$local_conan_server"; fi \
    && cd forwarder \
    && conan install --build=outdated ../forwarder_src/conan/conanfile.txt

# Second copy for everything so that the cached image can be used if only the source files change.
COPY ./ ../forwarder_src/

RUN cd forwarder \
    && cmake -DCONAN="MANUAL" --target="forward-epics-to-kafka" -DGOOGLETEST_DISABLE="ON" ../forwarder_src \
    && make -j4 forward-epics-to-kafka VERBOSE=1 \
    && apt-get remove --purge -y build-essential git python-pip cmake \
    && mv ../forwarder_src/docker_launch.sh /docker_launch.sh \
    && rm -rf ../forwarder_src /tmp/* /var/tmp/* /root/.conan/ /forwarder/src/*

CMD ["/docker_launch.sh"]
