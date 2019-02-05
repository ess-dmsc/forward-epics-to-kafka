from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE
from time import sleep
from helpers.producerwrapper import ProducerWrapper

CONFIG_TOPIC = "TEST_forwarderConfig"


def teardown_function(function):
    """
    Stops forwarder pv listening and resets any values in EPICS
    :param docker_compose: test fixture to apply to
    :return:
    """
    print("Resetting PVs", flush=True)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "")
    prod.stop_all_pvs()

    defaults = {
        PVDOUBLE: 0.0
    }

    for key, value in defaults.items():
        change_pv_value(key, value)
    sleep(3)


def test_logs_error_when_units_change(docker_compose_units_change):
    change_pv_value("{}.EGU".format(PVDOUBLE), "test1")

    change_pv_value(PVDOUBLE, 5)
    sleep(5)

    change_pv_value("{}.EGU".format(PVDOUBLE), "test2")

    change_pv_value(PVDOUBLE, 10)
    sleep(5)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "")
    prod.exit_forwarder()
    sleep(10)

    found = False
    test_string = "Units changed in PV SIMPLE:DOUBLE from test1 to test2"
    with open("logs/forwarder_tests_units.log", 'r') as file:
        for line in file.readlines():
            if test_string in line:
                found = True
                break

    assert found
