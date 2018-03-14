#!/usr/bin/env bash

/forwarder/forward-epics-to-kafka --broker ${BROKER:="localhost:9092"} \
  --broker-config ${STATUS_URI:="//localhost:9092/TEST_forwarderConfig"} \
  --graylog-logger-address ${GRAYLOG_ADDRESS:="localhost:12201"} -v}
