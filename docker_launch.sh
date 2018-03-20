#!/usr/bin/env bash

source /forwarder/activate_run.sh

/forwarder/forward-epics-to-kafka --broker ${KAFKA_BROKER:="localhost:9092"} \
  --broker-config ${CONFIG_URI:="//localhost:9092/TEST_forwarderConfig"} \
  -v
