from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.flatbuffer_helpers import check_expected_value
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE
from helpers.producerwrapper import ProducerWrapper

CONFIG_TOPIC = "TEST_forwarderConfig"


def teardown_function(function):
    """
    Stops forwarder pv listening and resets any values in EPICS
    """
    print("Resetting PVs", flush=True)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "")
    prod.stop_all_pvs()
    change_pv_value(PVDOUBLE, 0.0)


def test_forwarding_configured_from_file(docker_compose_config_from_json):
    # The test fixture starts the Forwarder with a JSON file that configures PVDOUBLE to be forwarded
    cons = create_consumer()
    cons.subscribe(["TEST_forwarderData_pv_from_config"])
    sleep(5)

    initial_value = 0

    # Change the PV value, so something is forwarded
    updated_value = 17
    change_pv_value(PVDOUBLE, updated_value)

    # Wait for PV to be updated
    sleep(5)
    # Check the initial value is forwarded
    first_msg, msg_key = poll_for_valid_message(cons)
    check_expected_value(first_msg, PVDOUBLE, initial_value)

    # Check the new value is forwarded
    second_msg, _ = poll_for_valid_message(cons)
    check_expected_value(
        second_msg, PVDOUBLE, updated_value
    )

    cons.close()
