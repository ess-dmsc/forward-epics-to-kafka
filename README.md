# Forward EPICS to Kafka

- Forward EPICS process variables to Kafka topics
- Listens to a Kafka topic for add/remove of forward mappings
- Kafka brokers configured as command line parameters
- Can forward so far the Epics 'NT' normative types, both scalar and array.
  What else is needed?


## Installation

Ansible playbook in ```./ansible``` even though not all dependencies are installed by
the playbook so far, see below, and in ```CMakeLists.txt```.


## Install together with configuration manager

- yum install gtest gtest-devel zeromq3 zeromq3-devel
- Note that as user of the library we still currently depend on redox as well


### Requirements

These libraries are expected in the ESS dev default locations or set via
environment variables (see src/CMakeLists.txt) and are so far not installed
by the ansible playbook:

- EPICS v4 (pvData and pvAccess)

Installed by the playbook via yum:

- cmake

Installed by the playbook currently under /opt/local until standardized:

- librdkafka
- flatbuffers
- RapidJSON
- fmt
  The EPEL in dev-env does not have it, so we assume the fmt git repository in /opt/local
  as used by the Ansible script, or wherever ENV[fmt_dir] points to.
  <https://github.com/fmtlib/fmt>

Tooling

- C++ compiler with c++11 support (Flatbuffers requires that as well)
- Doxygen if you want to make docs


### Environment variables

Location of compile-time dependencies can be controlled via environment variables
as well, see CMakeLists.


### Build (without playbook)

Optional, set CC and CXX environment to your liking.

Assuming you have 'make':

```
cmake <path-to-source>
make
make docs
```

### Performance

Currently limited by flatbuffer construction.  Optimization ongoing..



## Usage

A ```forward-epics-to-kafka --help``` shows the available options.
The forwarder will listen to a Kafka topic for configuration updates.
Configuration updates are JSON messages:

- Add a topic: ```{"cmd":"add", "channel":"[Epics-channel-name]", "topic":"[Kafka-topic-name]"}```
- Remove a topic: ```{"cmd":"remove", "channel":"[Epics-channel-name]"}```
- Exit the program: ```{"cmd":"exit"}```
- more to come



## Features planned for the future

Please send any feature requests you have (dominik.werder@psi.ch).

- More efficient threading
- Optimization of flat buffer creation
- Statistics and log
- Forward arbitrary Epics pvStructures
