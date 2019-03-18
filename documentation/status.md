# Status messages and command responses

## Status messages
The forwarder will publish regular status messages to the Kafka topic defined for status messages.

If all is fine and the forwarder is idle then the message would be something like:

```json
{
  "streams":[]
}
```

Once streams have been added then the status messages will show a list of what is being forwarded.
 
For example:

```json
{
  "streams":[
    {
      "channel_name":"SIMPLE:VALUE1",
      "converters":[
        {
          "broker":"localhost",
          "schema":"f142",
          "topic":"VALUE1"
        }
      ],
      "getQueueSize":0
    },
    {
      "channel_name":"SIMPLE:VALUE2",
      "converters":[
        {
          "broker":"localhost",
          "schema":"f142",
          "topic":"VALUE1"
        }
      ],
      "getQueueSize":0
    }
  ]
}
```