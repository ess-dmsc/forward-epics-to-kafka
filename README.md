# Forward EPICS to Kafka

- Forward EPICS process variables to Kafka topics
- Listens to a Kafka topic for add/remove of forward mappings
- Kafka brokers configured as command line parameters
- Can forward so far the Epics 'NT' normative types, both scalar and array
  What else is needed?


## Installation

Ansible playbook in ./ansible even though not all dependencies installed by default
so far, see below, and in CMakeLists.txt.
Uses cmake.


### Requirements

These libraries are expected in the ESS dev default locations or set via
environment variables (see src/CMakeLists.txt):
- EPICS v4 (pvData and pvAccess)
- Flatbuffers (Having flatc in PATH is a compile-time dependency)
- librdkafka

So far not in ESS dev:
- libjansson.  Expected in ```/opt/local/jansson-2.7-install``` which is also
  where the ansible playbook puts it.

Tooling
- C++ compiler with c++11 support (Flatbuffers requires that as well)
- Doxygen if you want to make docs


### Environment variables

Location of compile-time dependencies can be controlled via environment variables
as well, see CMakeLists.


### Build

Optional, set CC and CXX environment to your liking.

Assuming you have 'make':

```
cmake <path-to-source>
make
make docs
```

### Performance

Currently limited by flatbuffer construction.  Optimization ongoing..



## Features planned for the future

Please send any feature requests you have.

- More efficient threading
- Optimization of flat buffer creation
- Forward arbitrary Epics pvStructures
