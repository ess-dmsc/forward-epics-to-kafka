[![Build Status](https://jenkins.esss.dk/dm/job/ess-dmsc/job/forward-epics-to-kafka/job/master/badge/icon)](https://jenkins.esss.dk/dm/job/ess-dmsc/job/forward-epics-to-kafka/job/master/)
[![codecov](https://codecov.io/gh/ess-dmsc/forward-epics-to-kafka/branch/master/graph/badge.svg)](https://codecov.io/gh/ess-dmsc/forward-epics-to-kafka)
[![DOI](https://zenodo.org/badge/81432248.svg)](https://zenodo.org/badge/latestdoi/81432248)

# Forward EPICS to Kafka

## Summary
Application used at ESS to forward [EPICS](https://epics.anl.gov/) process
variables to [Kafka](https://kafka.apache.org/) topics.

## Features
- Converts EPICS data into FlatBuffers according to the configured schema
- New converters from EPICS to FlatBuffers can be easily
  [added](#adding-new-converter-plugins).
- FlatBuffer schemas are taken from repository `streaming-data-types`
- Comes with support for `f140_general` and `f142_logdata`
- One EPICS PV can be forwarded through [multiple](#forwarding-a-pv-through-multiple-converters)
  converters
- Conversion to FlatBuffers is distributed over threads
- The same converter instance can be [shared](#share-converter-instance-between-channels)
  between different channels

  - [Further documentation](documentation/README.md)
  - Details on installation see [here](documentation/INSTALLATION.md)
  - Details on performance see [here](documentation/PERFORMANCE.md)


# Tests

### Unit tests
Run the tests executable:

```
./tests/tests
```

### [Running System tests (link)](https://github.com/ess-dmsc/forward-epics-to-kafka/blob/master/system-tests/README.md)


## Deployment
TBD

## Built With

* [CMAKE](https://cmake.org/) - Cross platform makefile generation
* [Conan](https://conan.io/) - Package manager for C++

## Contributing

Please read [CONTRIBUTING.md] for details on the process for submitting pull requests to us, etc.

## Versioning

[Releases](https://github.com/ess-dmsc/forward-epics-to-kafka/releases)

## Authors

See also the list of [contributors](https://github.com/ess-dmsc/forward-epics-to-kafka/graphs/contributors) who participated in this project.

## License

This project is licensed under the BSD-2 - see the [LICENSE.md](LICENSE.md) file for details
