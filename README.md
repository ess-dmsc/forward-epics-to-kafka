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
- The same converter instance can be [shared](#share-converter-instance-between-channels)
  between different channels


## Installation

### Requirements

These libraries are expected in the ESS dev default locations or set via
environment variables (see `src/CMakeLists.txt`):

- EPICSv4
- `streaming-data-types`, easiest if cloned parallel to this repository.
  <https://github.com/ess-dmsc/streaming-data-types>
- pcre2 (e.g. `yum install pcre2 pcre2-devel` or `brew install pcre2`).
  pcre is supported equally well.

Tooling
- conan
- cmake (minimum tested is 2.8.11)
- C++ compiler with c++11 support
- Doxygen if you would like to `make docs`


### Conan repositories

The following remote repositories are required to be configured:

- https://api.bintray.com/conan/ess-dmsc/conan
- https://api.bintray.com/conan/conan-community/conan

You can add them by running

```
conan remote add <local-name> <remote-url>
```

where `<local-name>` must be substituted by a locally unique name. Configured
remotes can be listed with `conan remote list`.


### Build

Assuming you have `make`:

```
conan install <path-to-source>/conan --build=missing
cmake <path-to-source> [-DREQUIRE_GTEST=TRUE]
make
make docs  # optional
```


#### Running on macOS

When using Conan on macOS, due to the way paths to dependencies are handled,
the `activate_run.sh` file must be sourced before running the application. The
`deactivate_run.sh` can be sourced to undo the changes afterwards. This has not
been tested yet, and it is possible that EPICS libraries cannot be found.
Please report any issues you encounter when running this setup.


#### Dependencies in custom locations

The `forward-epics-to-kafka` follows standard `CMake` conventions.
You can use the standard `CMAKE_INCLUDE_PATH`, `CMAKE_LIBRARY_PATH` and
`CMAKE_PROGRAM_PATH` to point `CMake` into the right direction.
It will prefer dependencies found there over those in the system directories.

We of course also support the ESS EPICS installation scheme.
To that end, we use as specified in the ESS wiki:
- `EPICS_V4_BASE_VERSION`
- `EPICS_BASES_PATH`
- `EPICS_HOST_ARCH`
- `EPICS_MODULES_PATH`


#### Fully non-standard dependencies

If you like full control over the dependencies:

Here follows an example where all dependencies are in non-standard locations.
Also EPICS is a custom build from source.
No additional environment variables are needed.
Only a few basic dependencies (like PCRE) are in standard locations.

```
export D1=$HOME/software/;
export EPICS_MODULES_PATH=$D1/epics/EPICS-CPP-4.6.0;
export EPICS_HOST_ARCH=darwin-x86;
cmake \
-DREQUIRE_GTEST=1 \
-DCMAKE_INCLUDE_PATH="$D1/fmt;$D1/rapidjson/include;$D1/flatbuffers/include;$D1/librdkafka/include;$D1/googletest;$D1/epics/base-3.16.0.1/include" \
-DCMAKE_LIBRARY_PATH="$D1/librdkafka/lib;$D1/epics/base-3.16.0.1/lib/$EPICS_HOST_ARCH" \
-DCMAKE_PROGRAM_PATH="$D1/flatbuffers/bin" \
<path-to-forward-epics-to-kafka-repository>
```

Note that in this example, there is no need for `EPICS_V4_BASE_VERSION`
or `EPICS_BASES_PATH` because we give them explicitly in `CMAKE_*_PATH`.



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
lose updates.

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

Example which adds 2 EPICS PVs via `pva` (default) and a third EPICS variable
using `ca` Channel Access:

```
{
  "cmd": "add",
  "streams": [
    {
      "channel": "<EPICS PV name>",
      "converter": {
        "schema": "<schema-id>",
        "topic": "<Kafka-topic>"
      }
    },
    {
      "channel": "<EPICS PV name..>",
      "converter": {
        "schema": "<schema-id>",
        "topic": "//<host-if-we-do-not-like-the-default-host>[:port]/<Kafka-topic..>"
      }
    },
    {
      "channel": "<EPICS Channel Access channel name>",
      "channel_provider_type": "ca",
      "converter": {
        "schema": "<schema-id>",
        "topic": "<Kafka-topic..>"
      }
    }
  ]
}
```

The `topic` in the above stream configuration can contain the Kafka broker
hostname like `//<host>[:port]/<topic>` otherwise the default boker given in
the configuration file or on the command line is used.


Exit the forwarder:

```
{"cmd": "exit"}
```


### Using a configuration file

The forwarding can be also set up with a configuration file:

```bash
./forward-epics-to-kafka --config-file <your-file>
```

with e.g:

```json
{
	"broker": "//kafkabroker:9092",
	"status-uri": "//kafkabroker:9092/the_status_topic",
	"streams": [
		{
			"channel": "Epics_PV_name",
			"converter": { "schema": "f142", "topic": "Kafka_topic_name" }
		}
	]
}
```

All entries in the configuration file are optional.
The following keys can be set in the configuration file at the top level.
Given are the defaults.

- `broker` (string)
  - `//localhost:9092`
  - Default Kafka host to send the converted data to.

- `broker-config` (string)
  - `//localhost:9092/forward_epics_to_kafka_commands`
  - URI of the Kafka topic which should be monitored for commands.

- `status-uri` (string)
  - `(empty)`
  - URI of the Kafka topic where it should produce status messages.

- `conversion-threads` (int)
  - 1
  - Number of worker threads for the EPICS to FlatBuffers conversion.

- `conversion-worker-queue-size` (int)
  - 1024
  - Maximum queue size of each conversion worker thread.

- `main-poll-interval` (int, milliseconds)
  - 500
  - Interval for main loop maintenance tasks.



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
but you probably want to add it to `CMakeLists.txt`.
There will be support for dynamic loading of shared objects also soon.
Beware that converter instances are used from different threads.  If the
converter instance has state, it must take care of thread safety itself.



## Share Converter Instance between Channels

The same converter instance can be shared for usage on different channels.
This allows one converter instance to process events coming from different
EPICS channels.
Beware that converter instances are used from different threads.  If the
converter instance has state, it must take care of thread safety itself.

Example:
```json
{
  "streams": [
    {
      "channel": "Epics_PV_One",
      "converter": {"schema": "f142", "name": "my-named-conv", "topic": "Kafka_topic_name" }
    },
    {
      "channel": "Epics_PV_Two",
      "converter": {"schema": "f142", "name": "my-named-conv", "topic": "Kafka_topic_name" }
    }
  ]
}
```



## Features Coming Soon

- Configure options for threading (number of workers, queue lengths, ...)
- More dynamic scheduling of work
- Dynamically load converter plugins from shared objects


## For the future

Please send any feature requests you have (dominik.werder@psi.ch).

- Optionally read from (the future) configuration service
