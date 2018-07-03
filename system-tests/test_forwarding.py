from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata import LogData, Value, Int, Double, String
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.flatbuffer_helpers import check_double_value_and_equality,\
    check_message_pv_name_and_value_type, create_flatbuffers_object
from helpers.epics_helpers import change_pv_value


CONFIG_TOPIC = "TEST_forwarderConfig"


def test_config_file_channel_created_correctly(docker_compose):
    """
    Test that the channel defined in the config file is created.

    :param docker_compose: Test fixture
    :return: none
    """
    # Change the PV value, so something is forwarded
    change_pv_value("SIM:Phs", 10)
    # Wait for PV to be updated
    sleep(5)

    cons = create_consumer()
    cons.subscribe(['TEST_forwarderData_pv_from_config'])

    # Check the initial value is forwarded
    first_msg = poll_for_valid_message(cons).value()
    log_data_first = LogData.LogData.GetRootAsLogData(first_msg, 0)
    check_message_pv_name_and_value_type(log_data_first, Value.Value.Double, b'SIM:Phs')
    check_double_value_and_equality(log_data_first, 0)

    # Check the new value is forwarded
    second_msg = poll_for_valid_message(cons).value()
    log_data_second = LogData.LogData.GetRootAsLogData(second_msg, 0)
    check_message_pv_name_and_value_type(log_data_second, Value.Value.Double, b'SIM:Phs')
    check_double_value_and_equality(log_data_second, 10)

    cons.close()


def test_forwarder_sends_pv_updates_single_pv_double(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.
    
    :param docker_compose: Test fixture
    :return: none
    """
    data_topic = "TEST_forwarderData_double_pv_update"
    pvs = ["SIM:Spd"]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()

    # Update value
    change_pv_value("SIM:Spd", 5)
    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    first_msg = poll_for_valid_message(cons).value()
    log_data_first = LogData.LogData.GetRootAsLogData(first_msg, 0)
    check_message_pv_name_and_value_type(log_data_first, Value.Value.Double, b'SIM:Spd')
    check_double_value_and_equality(log_data_first, 0)

    second_msg = poll_for_valid_message(cons).value()
    log_data_second = LogData.LogData.GetRootAsLogData(second_msg, 0)
    check_message_pv_name_and_value_type(log_data_second, Value.Value.Double, b'SIM:Spd')
    check_double_value_and_equality(log_data_second, 5)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_string(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    :param docker_compose: Test fixture
    :return: none
    """
    data_topic = "TEST_forwarderData_string_pv_update"
    write_PV_name = "SIM:CmdS"
    read_PV_name = "SIM:CmdL"
    pvs = [read_PV_name]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()
    # Update value
    stop_command = b'stop'
    change_pv_value(write_PV_name, stop_command)

    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    # Poll for empty update - initial value of CmdL is nothing
    poll_for_valid_message(cons).value()
    # Poll for message which should contain forwarded PV update
    data_msg = poll_for_valid_message(cons).value()
    log_data = LogData.LogData.GetRootAsLogData(data_msg, 0)

    check_message_pv_name_and_value_type(log_data, Value.Value.String, b'SIM:CmdL')

    union_string = String.String()
    union_string.Init(log_data.Value().Bytes, log_data.Value().Pos)
    assert union_string.Value() == stop_command

    cons.close()
