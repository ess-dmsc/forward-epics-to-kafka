FROM dmscid/epics-base

RUN apk add --no-cache git gcc g++ linux-headers cmake make python py-pip python-dev bash

RUN pip install --no-cache-dir conan
RUN git clone -b master https://github.com/ess-dmsc/streaming-data-types.git

RUN mkdir forwarder
COPY ./conan /forwarder/conan

RUN conan remote add ess-dmsc https://api.bintray.com/conan/ess-dmsc/conan && \
    conan remote add conan-community https://api.bintray.com/conan/conan-community/conan && \
    conan install /forwarder/conan --build=missing

ADD . /forwarder

RUN . /etc/profile

RUN mkdir build && cd build

RUN echo ${EPICS_BASE}

RUN cmake ../forwarder

RUN make --directory=./build VERBOSE=1
