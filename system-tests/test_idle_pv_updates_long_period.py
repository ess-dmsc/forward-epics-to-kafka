from pytest import raises
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.f142_logdata import LogData, Value, Double
from helpers.flatbuffer_helpers import check_expected_values
from time import sleep
from helpers.PVs import PVDOUBLE


def test_forwarder_does_not_send_pv_update_more_than_once_when_periodic_update_is_used(docker_compose_idle_updates_long_period):
    consumer = create_consumer()
    data_topic = "TEST_forwarderData_long_idle_updates"
    consumer.subscribe([data_topic])
    sleep(3)
    msg = poll_for_valid_message(consumer)
    check_expected_values(msg, Value.Value.Double, PVDOUBLE, 0)

    with raises(AssertionError):
        # AssertionError because there are no more messages to poll
        poll_for_valid_message(consumer)
