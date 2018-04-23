### System tests for forward-epics-to-kafka


To install the CaChannel library with epics environment variables in a virtual environment you will need to use the virtual environment's pip passed with sudo -E to preserve environment variables.

these should be: 
EPICS_BASE 
EPICS_HOST_ARCH

Currently, the tests rely on the flatbuffers f142 schema to be compiled for Python.

CaChannel docs - https://cachannel.readthedocs.io/en/latest/index.html
confluent-kafka docs - https://docs.confluent.io/current/clients/confluent-kafka-python/#

