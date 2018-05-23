#!/usr/bin/env bash

echo "Activating forwarder run environment"

source /forwarder/activate_run.sh

echo "Launching forwarder"

/forwarder/forward-epics-to-kafka --broker ${KAFKA_BROKER:="localhost:9092"} \
  --broker-config ${CONFIG_URI:="//localhost:9092/TEST_forwarderConfig"} --pv-update-period ${PERIOD_MS:="2000"}
