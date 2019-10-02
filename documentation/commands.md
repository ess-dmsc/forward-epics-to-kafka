# Commands

Commands in the form of JSON messages are used configure which PVs are forwarded and how.

Commands are generally sent through Kafka via the broker and topic specified by the
`--command-uri` option; however, commands can also be given in the [configuration
file](Commands via the configuration file).

Note: some example commands can be found in the system tests.

## Add PVs to the list to be forwarded

The add PVs command consists of the following parameters which are defined as key-value pairs in JSON:

- cmd: The command name, must be `add`
- streams: The PV streams to add. This is a list of dictionaries where each dictionary represents one PV.

An example command that adds two EPICS PVs via PV Access (pva, this is the default) and a third EPICS variable using 
Channel Access (ca):

```json
{
  "cmd": "add",
  "streams": [
    {
      "channel": "MYIOC:VALUE1",
      "converter": {
        "schema": "f142",
        "topic": "my_kafka_server:9092/some_topic"
      }
    },
    {
      "channel": "MYIOC:VALUE2",
      "converter": {
        "schema": "TdcTime",
        "topic": "my_kafka_server:9092/some_topic2"
      }
    },
    {
      "channel": "MYIOC:VALUE3",
      "channel_provider_type": "ca",
      "converter": {
        "schema": "f142",
        "topic": "my_kafka_server:9092/some_topic"
      }
    }
  ]
}
```

The parameters for defining a PV to be forwarded are:

- channel: The EPICS PV name
- converter: Dictionary for defining how to process the PV value:
    * schema: The FlatBuffers schema to use to encode the value
    * topic: The Kafka topic to send the encoded value to

### Multiple converters
It is also possible to provide multiple converters for PV so as to allow a PV to be encoded differently and/or 
sent to multiple Kafka topics or servers. 

For example:

```json
# Rest of add command skipped for brevity
{
  "channel": "MYIOC:VALUE1",
  "converter": [
    {
      "schema": "f142",
      "topic": "my_kafka_server:9092/some_topic"
    },
    {
      "schema": "fXXX",                                     # Different schema used
      "topic": "my_other_kafka_server:9092/some_topic2"   # Send to a different topic on a different server
    }
  ]
}
```

The following converters have been implemented so far:
#### *f142* General EPICS data
Can convert most EPICS update structure types and put the values into *f142* schema flatbuffers.

#### *TdcTime* Chopper TDC timestamp
Should only be used for converting chopper TDC timestamp updates in the form of arrays with size *n * 2*. The array
elements must be integers with a size of 32 bits (int32/`epics::pvData::ScalarType::pvInt`). Arrays that are of size 0
are ignored. The timestamps are put in flatbuffers of type *tdct*.

### Shared converters (advanced)
The same converter instance can be shared across different channels. This allows one converter instance to process 
events coming from multiple EPICS channels. 
This is done by naming converters and providing the name for the required channels.

For example:

```json
# Rest of add command skipped for brevity
"streams": [
    {
      "channel": "MYIOC:VALUE1",
      "converter": {
        "schema": "f142", 
        "name": "shared_converter", 
        "topic": "my_kafka_server:9092/some_topic" 
      }
    },
    {
      "channel": "MYIOC:VALUE2",
      "converter": {
      "schema": "f142", 
        "name": "shared_converter", 
        "topic": "my_kafka_server:9092/some_topic2" 
      }
    }
]
```

Beware that converter instances are used from different threads. If the converter instance has state, it must take care 
of thread safety itself.

## Stop a PV from being forwarded
The stop command consists of the following JSON key-value pairs:
- cmd: The command name, must be `stop_channel`
- channel: The EPICS PV of the stream to stop

For example:

```json
{
  "cmd": "stop_channel",
  "channel": "MYIOC:VALUE1"
}
```

## Stop all PVs from being forwarded
The stop all command stops the forwarding of all the PVs being forwarded; in other words, it clears the list of PVs being
forwarded.

The JSON for the command is:

```json
{"cmd": "stop_all"}
```

## Exit the forwarder
This command causes the forwarder to be shutdown gracefully.

The JSON for the command is:

```json
{"cmd": "exit"}
```
