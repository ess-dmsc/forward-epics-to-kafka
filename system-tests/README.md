## Forward-epics-to-kafka system tests 

### How to run the tests

[optional] Set up a Python virtual environment and activate it (see [here](https://virtualenv.pypa.io/en/stable/))

* Install the requirements using pip: `pip install -r system-tests/requirements.txt`

* Compile the `f142_logdata` flatbuffers schema with `flatc --python -o /system-tests/helpers/f142_logdata`

* Run `pytest system-tests/test_example.py`
