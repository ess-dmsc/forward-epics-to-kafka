#!/usr/bin/env bash

echo "Launching forwarder"

export LD_LIBRARY_PATH=/forwarder/lib/

/forwarder/bin/forward-epics-to-kafka --config-file=${CONFIG_FILE:="forwarder_config.ini"}
