FROM ubuntu:18.04

ARG http_proxy
ARG https_proxy
ARG local_conan_server

ADD "https://raw.githubusercontent.com/ess-dmsc/docker-ubuntu18.04-build-node/7a4840422e78d451ada97662399116c31a4bc44e/files/default_profile" "/root/.conan/profiles/default"

COPY conan/ ../forwarder_src/conan/

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update -y \
    && apt-get --no-install-recommends -y install build-essential git python3 python3-pip python3-setuptools cmake tzdata vim \
    && apt-get -y autoremove \
    && apt-get clean all \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --upgrade pip \
    && pip3 install conan \
    && rm -rf /root/.cache/pip/* \
    && mkdir forwarder \
    && conan config install http://github.com/ess-dmsc/conan-configuration.git \
    && if [ ! -z "$local_conan_server" ]; then conan remote add --insert 0 ess-dmsc-local "$local_conan_server"; fi \
    && cd forwarder \
    && conan install --build=outdated ../forwarder_src/conan/conanfile.txt

# Second copy for everything so that the cached image can be used if only the source files change.
COPY ./ ../forwarder_src/

RUN cd forwarder \
    && cmake -DCONAN="MANUAL" --target="forward-epics-to-kafka" -DBUILD_TESTS="OFF" ../forwarder_src \
    && make -j4 forward-epics-to-kafka \
    && apt-get remove --purge -y build-essential git python3 python3-pip python3-setuptools cmake \
    && mv ../forwarder_src/docker_launch.sh /docker_launch.sh \
    && rm -rf ../forwarder_src /tmp/* /var/tmp/* /forwarder/src/* \
    && cd /root/.conan/data/ \
    && ls | grep -v epics | xargs rm -rf \
    && rm -rf /root/.conan/data/epics/*/ess-dmsc/stable/build/*/EPICS-CPP*

CMD ["/docker_launch.sh"]
