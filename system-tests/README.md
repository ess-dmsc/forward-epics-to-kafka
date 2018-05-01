## Forward-epics-to-kafka system tests 

### How to run the tests

[requirement] Currently a local EPICS installation is needed.

[optional] Set up a Python virtual environment and activate it (see [here](https://virtualenv.pypa.io/en/stable/))

* Set the EPICS environment variables : `EPICS_BASE` and `EPICS_HOST_ARCH`. For example:
```
export EPICS_BASE=/opt/epics/bases/base-3.15.5
export EPICS_HOST_ARCH=linux-x86_64
```

* Install the requirements using pip: `pip install -r system-tests/requirements.txt`

* Compile the `f142_logdata` flatbuffers schema with `flatc --python` into `/system-tests/helpers/f142_logdata`

* Run `pytest system-tests/test_example.py`
