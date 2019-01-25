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
			"converter": { "schema": "f142", "topic": "//localhost:9092/Kafka_topic_name" }
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
        "topic": "//<host>[:port]<Kafka-topic>"
      }
    },
    {
      "channel": "<EPICS PV name..>",
      "converter": {
        "schema": "<schema-id>",
        "topic": "//<host>[:port]/<Kafka-topic..>"
      }
    },
    {
      "channel": "<EPICS Channel Access channel name>",
      "channel_provider_type": "ca",
      "converter": {
        "schema": "<schema-id>",
        "topic": "//<host>[:port]/<Kafka-topic..>"
      }
    }
  ]
}
```


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
    { "schema": "f142", "topic": "//localhost:9092/Kafka_topic_name" },
    { "schema": "f999", "topic": "//otherhost:9092/some_other_topic" }
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
      "converter": {"schema": "f142", "name": "my-named-conv", "topic": "//host:port/Kafka_topic_name" }
    },
    {
      "channel": "Epics_PV_Two",
      "converter": {"schema": "f142", "name": "my-named-conv", "topic": "//host:port/Kafka_topic_name" }
    }
  ]
}
```
