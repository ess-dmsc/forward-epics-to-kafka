# Forward EPICS to Kafka

- Forward EPICS process variables to Kafka topics
- Translates EPICS data into FlatBuffers according to the configured schema
- New translations from EPICS to FlatBuffers can be easily registered
- FlatBuffer schemas are taken from repository `streaming-data-types`
- Comes with support for `f140_general` and `f142_logdata`


## Installation

### Requirements

These libraries are expected in the ESS dev default locations or set via
environment variables (see `src/CMakeLists.txt`):

- EPICSv4
- librdkafka
- flatbuffers (headers and `flatc` executable)
- RapidJSON (header only)
- fmt (`fmt/format.h` and `fmt/format.cc`) <https://github.com/fmtlib/fmt>
- `streaming-data-types`, easiest if cloned parallel to this repository.
  <https://github.com/ess-dmsc/streaming-data-types>
- `graylog_logger` <https://github.com/ess-dmsc/graylog-logger>
- pcre2 (e.g. `yum install pcre2 pcre2-devel` or `brew install pcre2`)

Tooling
- cmake (above 2.8.11)
- C++ compiler with c++11 support
- Doxygen if you would like to make docs

Others (optional)
- Google Test  (clone the gtest repository parallel to this repository)


### Build

Assuming you have `make`
```
cmake <path-to-source>
make
make docs
```


### Tests

Run
```
./tests/tests
```


### Performance

Some more thorough figures are to be included here, but we have forwarded
about 200MB/s without problems.


## Usage

```
forward-epics-to-kafka --help
```

The forwarder will listen to the Kafka topic given on the command line for
commands.  Configuration updates are JSON messages.  For example:

Add a topic:
```
{"cmd":"add", "streams": [
  {"channel": "<EPICS PV name>", "converter": {
    "schema":"<schema-id>", "topic":"<Kafka-topic>"}
  },
  {"channel": "<EPICS PV name..>", "converter": {
    "schema":"<schema-id>", "topic":"<Kafka-topic..>"}
  }
]}
```

Exit the forwarder:
```
{"cmd": "exit"}
```


### Using a configuration file

The forwarding can be also set up with a configuration file, e.g:
```json
{
	"broker": "kafkabroker:9092",
	"streams": [
		{
			"channel": "Epics_PV_name",
			"converter": { "schema": "f142", "topic": "Kafka_topic_name" }
		}
	]
}
```

#### More options possible in the configuration file
These are optional.
Given are the defaults:
```json
{
  "conversion-threads": 1,
  "conversion-worker-queue-size": 1024
}
```



## Features planned for the future

Please send any feature requests you have (dominik.werder@psi.ch).

- More options for threading
- Multiple converters per epics channel
- Pinned converters:  Multiple channels can be routed to the same converter
- Optionally read from (the future) configuration service


## Release notes

### Breaking change around 2017-03-20

EpicsPVUpdate changed, access via `epics_pvstr->pvstr`
