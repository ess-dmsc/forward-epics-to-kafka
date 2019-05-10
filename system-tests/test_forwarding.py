from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata.Value import Value
from helpers.ep00 import EventType
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message, get_all_available_messages, \
    poll_for_connection_status_message
from helpers.flatbuffer_helpers import check_expected_values, check_multiple_expected_values, \
    check_expected_connection_status_values
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE, PVENUM

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
    first_msg, msg_key = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0.0)
    assert(msg_key == b'SIMPLE:DOUBLE'), 'Message key expected to be the same as the PV name'
    # We set the message key to be the PV name so that all messages from the same PV are sent to
    # the same partition by Kafka. This ensures that the order of these messages is maintained to the consumer.

    # Check the new value is forwarded
    second_msg, _ = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 10.0)
    cons.close()


def test_connection_status_messages(docker_compose):
    """
      GIVEN PV is configured to be forwarded
      WHEN Connection status changes
      THEN Forwarder publishes ep00 message with connection status

      NOTE: Enums are converted to Ints in the forwarder.
      """
    data_topic = "TEST_forwarderData_connection_status"
    pvs = [PVENUM]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(5)

    cons = create_consumer()

    # Update value
    change_pv_value(PVENUM, "START")
    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    first_msg = poll_for_connection_status_message(cons)
    check_expected_connection_status_values(first_msg, EventType.EventType.CONNECTED)

    cons.close()
