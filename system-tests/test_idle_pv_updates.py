from helpers.kafkahelpers import create_consumer, poll_for_valid_message
from helpers.f142_logdata import LogData, Value, Double
from helpers.flatbufferhelpers import check_message_pv_name_and_value_type, check_double_value_and_equality
from helpers.epicshelpers import change_pv_value
from time import sleep


def test_forwarder_sends_idle_pv_updates(docker_compose_fake_epics):
    consumer = create_consumer()
    data_topic = "TEST_forwarderData_idle_updates"
    consumer.subscribe([data_topic])
    sleep(2)
    change_pv_value("SIM:ParkAng", 10)
    sleep(5)
    for i in range(3):
        msg = poll_for_valid_message(consumer).value()
        log_data_first = LogData.LogData.GetRootAsLogData(msg, 0)
        check_message_pv_name_and_value_type(log_data_first, Value.Value.Double, b'SIM:ParkAng')
        check_double_value_and_equality(log_data_first, 10)
        sleep(3)
