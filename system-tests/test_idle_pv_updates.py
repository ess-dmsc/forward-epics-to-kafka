from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.f142_logdata import LogData, Value, Double
from helpers.flatbuffer_helpers import check_message_pv_name_and_value_type, check_double_value_and_equality
from time import sleep
from PVs import PVDOUBLE


def test_forwarder_sends_idle_pv_updates(docker_compose_idle_updates):
    consumer = create_consumer()
    data_topic = "TEST_forwarderData_idle_updates"
    consumer.subscribe([data_topic])
    sleep(10)
    for i in range(3):
        msg = poll_for_valid_message(consumer)
        check_message_pv_name_and_value_type(msg, Value.Value.Double, PVDOUBLE)
        check_double_value_and_equality(msg, 0)
    consumer.close()
