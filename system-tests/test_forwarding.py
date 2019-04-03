from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata.Value import Value
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.flatbuffer_helpers import check_expected_values
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE, PVSTR, PVLONG, PVENUM


CONFIG_TOPIC = "TEST_forwarderConfig"


def teardown_function(function):
    """
    Stops forwarder pv listening and resets any values in EPICS
    """
    print("Resetting PVs", flush=True)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "")
    prod.stop_all_pvs()

    defaults = {
        PVDOUBLE: 0.0,
        # We have to use this as the second parameter for caput gets parsed as empty so does not change the value of
        # the PV
        PVSTR: "\"\"",
        PVLONG: 0,
        PVENUM: "INIT"
    }

    for key, value in defaults.items():
        change_pv_value(key, value)
    sleep(3)


def test_config_file_channel_created_correctly(docker_compose):
    """
    GIVEN Forwarder is started with a config file specifying a PV to forward (forwarder_config.json)
    WHEN PV value is updated
    THEN Forwarder publishes the update to Kafka

    :param docker_compose: Test fixture (see https://docs.pytest.org/en/latest/fixture.html)
    """
    cons = create_consumer()
    cons.subscribe(['TEST_forwarderData_pv_from_config'])
    sleep(5)
    # Change the PV value, so something is forwarded
    change_pv_value(PVDOUBLE, 10)
    # Wait for PV to be updated
    sleep(5)
    # Check the initial value is forwarded
    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0.0)

    # Check the new value is forwarded
    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 10.0)
    cons.close()


