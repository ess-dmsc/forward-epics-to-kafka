## Forward-epics-to-kafka system tests 

### How to run tests

[requirement] currently a local EPICS installation is needed.

[optional] set up a python virtual environment and activate it 

* Set environment variables : `EPICS_BASE` and `EPICS_HOST_ARCH`

* Install [py.test](https://docs.pytest.org/en/latest/getting-started.html), [docker-compose](https://pypi.org/project/docker-compose/), [confluent-kafka](https://pypi.org/project/confluent-kafka/), [flatbuffers](https://pypi.org/project/flatbuffers/) and [CaChannel](https://pypi.org/project/CaChannel/)

* Compile the `f142_logdata` flatbuffers schema with `flatc --python` into `/system-tests/helpers/f142_logdata`

* Run `pytest test_example.py`