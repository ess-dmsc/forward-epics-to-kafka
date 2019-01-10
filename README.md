[![Build Status](https://jenkins.esss.dk/dm/job/ess-dmsc/job/forward-epics-to-kafka/job/master/badge/icon)](https://jenkins.esss.dk/dm/job/ess-dmsc/job/forward-epics-to-kafka/job/master/)
[![codecov](https://codecov.io/gh/ess-dmsc/forward-epics-to-kafka/branch/master/graph/badge.svg)](https://codecov.io/gh/ess-dmsc/forward-epics-to-kafka)
[![DOI](https://zenodo.org/badge/81432248.svg)](https://zenodo.org/badge/latestdoi/81432248)

# Summary
Application used at ESS to forward [EPICS](https://epics.anl.gov/) process
variables to [Kafka](https://kafka.apache.org/) topics.

- [Further documentation](documentation/README.md)
- Details on install see [here](documentation/INSTALLATION.md)
- Details on performance see [here](documentation/PERFORMANCE.md)

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


The forwarder can be also set up with a configuration file:

```bash
./forward-epics-to-kafka --config-file <your-file>
```

with an `ini` file for command line options:

```ini
config-topic=//kakfabroker:9092/the_config_topic
status-topic=//kafkabroker:9092/the_status_topic
streams-json=./streams.json
kafka-config=consumer.timeout.ms 501 fetch.message.max.bytes 1234 api.version.request true
verbosity=5
```

and/or a `json` file for the list of streams to add:

```json
{
	"streams": [
		{
			"channel": "Epics_PV_name",
			"converter": { "schema": "f142", "topic": "Kafka_topic_name" }
		}
	]
}
```

All command line options should be passed through the command line or by using a `.ini`. The JSON file was previously responsible for some options however these are now available through the command line. This does mean separate files are required, however there is more distinction between streams and command line options. The JSON will also look similar to any command messages received through Kafka.


### Commands

The forwarder will listen to the Kafka topic given on the command line for
commands.  Configuration updates are JSON messages.

#### Add

Adds PVs to be forwarded to Kafka.

This example adds 2 EPICS PVs via `pva` (default) and a third EPICS variable
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
hostname like `//<host>[:port]/<topic>` otherwise the default broker given in
the configuration file or at the command line is used.

#### Stop channel

Stops a PV being forwarded to Kafka.

```
{
  "cmd": "stop_channel",
  "channel": "<EPICS PV name>"
}
```

#### Stop all

Stops all PVs from being forwarded to Kafka.

```
{"cmd": "stop_all"}
```

#### Exit

Exits the forwarder.

```
{"cmd": "exit"}
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





# Tests

### Unit tests
Run the tests executable:

```
./tests/tests
```

### [Running System tests (link)](https://github.com/ess-dmsc/forward-epics-to-kafka/blob/master/system-tests/README.md)


### Update Frequency

Note that EPICS is not made for very high frequency updates as it will discard updates if there are too many.

That being said, a process variable updated at 10 kHz containing 2048 doubles,
with 3 EPICS to Flatbuffer converters attached and therefore producing 460MB/s
of data works just fine, utilizing about 30% of each core on a reasonable desktop machine.

Higher frequency updates over EPICS should be batched into a PV structure which can hold multiple events at a time, such as a waveform record.

The Forwarder uses the [MDEL](https://epics.anl.gov/EpicsDocumentation/AppDevManuals/RecordRef/Recordref-5.html#MARKER-9-15) monitor specification for monitoring PV updates rather than the ADEL Archive monitoring specification. This means that every PV update is processed rather than just those that exceed the ADEL.

#### Idle PV Updates

To enable the forwarder to publish PV values periodically even if their values have not been updated use the `pv-update-period <MILLISECONDS>` flag. This runs alongside the normal PV monitor so it will push value updates as well as sending values periodically.

By default this is not enabled.



## Adding New Converter Plugins

New converters from EPICS to Flatbuffers can be easily added.
Please have a look at the last 20 lines of `src/schemas/f142/f142.cxx` on how
to register your plugin with the SchemaRegistry.
There is no need to touch existing code at all to register a new plugin,
but you probably want to add it to `CMakeLists.txt`.
There will be support for dynamic loading of shared objects also soon.
Beware that converter instances are used from different threads.  If the
converter instance has state, it must take care of thread safety itself.

## Features Requests
Feel free to create a GitHub issue if you have a feature request or, even better, implement it
yourself on a branch and create a pull request!

## Contributing
See CONTRIBUTING.md
