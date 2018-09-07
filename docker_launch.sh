#!/usr/bin/env bash

echo "Activating forwarder run environment"

source /forwarder/activate_run.sh

echo "Launching forwarder"

/forwarder/forward-epics-to-kafka --config-file=${CONFIG_FILE:="forwarder_config.ini"} --fake-pv-period=${FAKE_PV_PERIOD_MS:="0"} --pv-update-period=${IDLE_PV_UPDATE_PERIOD:="0"}
