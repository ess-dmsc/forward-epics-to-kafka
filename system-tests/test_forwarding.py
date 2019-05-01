from helpers.f142_logdata.Value import Value
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.flatbuffer_helpers import check_expected_values
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE

CONFIG_TOPIC = "TEST_forwarderConfig"


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
    first_msg, _ = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0.0)

    # Check the new value is forwarded
    second_msg, _ = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 10.0)
    cons.close()
