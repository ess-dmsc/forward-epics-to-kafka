# Forward EPICS to Kafka

- Forward EPICS process variables to Kafka topics
- Converts EPICS data into FlatBuffers according to the configured schema
- New converters from EPICS to FlatBuffers can be easily
  [added](#adding-new-converter-plugins).
- FlatBuffer schemas are taken from repository `streaming-data-types`
- Comes with support for `f140_general` and `f142_logdata`
- One EPICS PV can be forwarded through [multiple](#forwarding-a-pv-through-multiple-converters)
  converters
- Conversion to FlatBuffers is distributed over threads


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

#### Tests with actual traffic
The tests which involve actual EPICS and Kafka traffic are disabled by default.
They can be run with:
```
./tests/tests -- --gtest_filter=Remote
```
Please note that you probably have to specify your broker, so a more complete
command looks like:
```
./tests/tests --broker //<host> --broker-config //<host>/tmp-commands -- --gtest_filter=Remote\*
```
Please note also that you need to have an EPICS PV running:
- Normative Types Array Double, name: `forwarder_test_nt_array_double`
- Normative Types Array Int32, name: `forwarder_test_nt_array_int32`
and they need to update during the runtime of the test.



## Performance

Some more thorough figures are to be included here, but we have forwarded
about 200MB/s without problems for extended periods of time.
Higher bandwidth has been done, but not yet tested over long time periods.

### Conversion bandwidth

If we run as usual except that we do not actually write to Kafka, tests show
on my quite standard 4 core desktop PC a flatbuffer conversion rate of about
2.8 GB/s. with all cores at a ~80% usage.
These numbers are of course very rough estimates and depend on a lot of
factors.  For systematic tests are to be done.

### Update Frequency

Note that EPICS is not made for very high frequency updates as it will happily
loose updates.

That being said, a process variable updated at 10 kHz containing 2048 doubles,
with 3 EPICS to flatbuffer converters attached and therefore producing 460MB/s
of data works just fine, utilizing about 30% of each core on my desktop machine
including the EPICS producer.

Higher frequency updates over EPICS should be batched into a PV which contains
many events at a time.



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


### Forwarding a PV through Multiple Converters
If you pass an array of converters instead, the EPICS PV will be forwarded
through multiple converters:
```
{
  "channel": "Epics_PV_name",
  "converter": [
    { "schema": "f142", "topic": "Kafka_topic_name" },
    { "schema": "f999", "topic": "some_other_topic" }
  ]
}
```


## Adding New Converter Plugins

New converters from EPICS to Flatbuffers can be easily added.
Please have a look at the last 20 lines of `src/schemas/f142/f142.cxx` on how
to register your plugin with the SchemaRegistry.
There is no need to touch existing code at all to register a new plugin,
but you probably want to at it to `CMakeLists.txt`.
There will be support for dynamic loading of shared objects also soon.



## Features Coming Soon

- Pinned converters:  Multiple channels can be routed to the same converter
- Configure options for threading (number of workers, queue lengths, ...)
- More dynamic scheduling of work
- Dynamically load converter plugins from shared objects


## For the future

Please send any feature requests you have (dominik.werder@psi.ch).

- Optionally read from (the future) configuration service



## Release notes
