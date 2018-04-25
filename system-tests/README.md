## System tests for forward-epics-to-kafka

### Requirements

#### Python libraries:

[py.test](https://docs.pytest.org/en/latest/getting-started.html) - [docs](https://docs.pytest.org/en/latest/contents.html#toc)

[docker-compose](https://pypi.org/project/docker-compose/)

[CaChannel](https://pypi.org/project/CaChannel/) - [docs](https://cachannel.readthedocs.io/en/latest/index.html)

[confluent-kafka](https://pypi.org/project/confluent-kafka/) -  [docs](https://docs.confluent.io/current/clients/confluent-kafka-python/#)

##### CaChannel and EPICS

To install the CaChannel library with epics environment variables in a virtual environment you will need to use the virtual environment's pip passed with sudo -E to preserve environment variables.

these should be:
 
EPICS_BASE
 
EPICS_HOST_ARCH

#### Flatbuffers
Currently, the tests rely on the Flatbuffers f142 schema to be compiled for Python in the `f142_logdata` directory.


### How to run

run `/system-tests/test_example.py` with py.test, if in an IDE using `-s` seems to make it prettier. 

To run docker containers manually use docker-compose with `/system-tests/docker-compose.yml`
