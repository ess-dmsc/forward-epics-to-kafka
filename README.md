[![Build Status](https://jenkins.esss.dk/dm/job/ess-dmsc/job/forward-epics-to-kafka/job/master/badge/icon)](https://jenkins.esss.dk/dm/job/ess-dmsc/job/forward-epics-to-kafka/job/master/)
[![codecov](https://codecov.io/gh/ess-dmsc/forward-epics-to-kafka/branch/master/graph/badge.svg)](https://codecov.io/gh/ess-dmsc/forward-epics-to-kafka)
[![DOI](https://zenodo.org/badge/81432248.svg)](https://zenodo.org/badge/latestdoi/81432248)

# Forward EPICS to Kafka

Forwards EPICS PVs to Apache Kafka. Part of the ESS data streaming pipeline.

## Usage

```
  -h,--help                   Print this help message and exit
  --log-file TEXT             Log filename
  --streams-json TEXT         Json file for streams to add
  --kafka-gelf TEXT           Kafka GELF logging //broker[:port]/topic
  --graylog-logger-address TEXT
                              Address for Graylog logging
  --influx-url TEXT           Address for Influx logging
  -v,--verbosity INT=3        Syslog logging level
  --config-topic URI=//localhost:9092/forward_epics_to_kafka_commands (REQUIRED)
                              <//host[:port]/topic> Kafka host/topic to listen for commands on
  --status-topic URI          <//host[:port][/topic]> Kafka broker/topic to publish status updates on
  --pv-update-period UINT=0   Force forwarding all PVs with this period even if values are not updated (ms). 0=Off
  --fake-pv-period UINT=0     Generates and forwards fake (random value) PV updates with the specified period in milliseconds, instead of forwarding real PV updates from EPICS
  --conversion-threads UINT=1 Conversion threads
  --conversion-worker-queue-size UINT=1024
                              Conversion worker queue size
  --main-poll-interval INT=500
                              Main Poll interval
  -S,--kafka-config KEY VALUE ...
                              LibRDKafka options
  -c,--config-file TEXT       Read configuration from an ini file
```

### Configuration Files

The forwarder can be configured from a file via `--config-file <ini>` which mirrors the command line options.

For example:

```ini
config-topic=//<host>[:port]/the_config_topic
status-topic=//<host>[:port]/the_status_topic
streams-json=./streams.json
kafka-config=consumer.timeout.ms 501 fetch.message.max.bytes 1234 api.version.request true
verbosity=5
```

Note: the Kafka options are key-value pairs and the forwarder can be given multiple by appending the key-value pair to 
the end of the command line option.

### Sending commands to the file-writer

Beyond the configuration options given at start-up, the forwarder can be sent commands via Kafka to configure which PVs 
are forwarded.

See [commands](documentation/commands.md) for more information.

## Installation

The supported method for installation is via Conan.

### Prerequisites

The following minimum software is required to get started:

- Conan
- CMake >= 3.1.0
- Git
- A C++14 compatible compiler (preferably GCC or Clang)
- Doxygen (only required if you would like to generate the documentation)

Conan will install all the other required packages.

### Add the Conan remote repositories

Add the required remote repositories like so:

```bash
conan remote add conancommunity https://api.bintray.com/conan/conan-community/conan
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
conan remote add conan-transit https://api.bintray.com/conan/conan/conan-transit
conan remote add ess-dmsc https://api.bintray.com/conan/ess-dmsc/conan
```

### Build

From within the forwarder's top directory:

```bash
mkdir _build
cd _build
conan install ../conan --build=missing
cmake ..
make
```

There are additional CMake flags for adjusting the build:
* `-DRUN_DOXYGEN=TRUE` if Doxygen documentation is required. Also, requires `make docs` to be run afterwards
* `-DBUILD_TESTS=FALSE` to skip building the unit tests

### Running the unit tests

From the build directory:

```bash
./tests/tests
```

### Running on OSX

When using Conan on OSX, due to the way paths to dependencies are handled,
the `activate_run.sh` file must be sourced before running the application. The
`deactivate_run.sh` can be sourced to undo the changes afterwards.

### System tests

The system tests consist of a series of automated tests for this repository that test it in ways similar to how it would 
be used in production.

See [System Tests page](system-tests/README.md) for more information.

## Documentation

See the `documentation` directory.

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for information on submitting pull requests to this project.

## License

This project is licensed under the BSD 2-Clause "Simplified" License - see the [LICENSE.md](LICENSE.md) file for details.
