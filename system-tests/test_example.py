from helpers.producerwrapper import ProducerWrapper
from confluent_kafka import Producer
from helpers.f142_logdata import LogData, Value, Int, Double, String
from json import loads
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


def test_config_created_correctly(docker_compose):
    """
    Test we can send a configuration message through kafka.
    
    :param docker_compose: Test fixture
    :return: none
    """
    topic_suffix = "_topic_exists_on_creation"
    server = "localhost:9092"
    data_topic = "TEST_forwarderData" + topic_suffix
    pvs = ["SIM:Spd", "SIM:ActSpd"]
    prod = ProducerWrapper(server, CONFIG_TOPIC + topic_suffix, data_topic)
    prod.add_config(pvs)
    cons = create_consumer()
    cons.subscribe([CONFIG_TOPIC + topic_suffix])
    msg = poll_for_valid_message(cons)
    buf_json = loads(str(msg.value(), encoding="utf-8"))
    check_json_config(buf_json, data_topic, pvs)
    cons.close()


def test_flatbuffers_encode_and_decode(docker_compose):
    """
    Test the flatbuffers schema gets pushed to kafka then pulled back and decompiled.
    
    Note: this is more for documentation purposes than a system test as it shows how to construct a flatbuffers
    object.
    
    :param docker_compose: Test fixture
    :return: none
    """
    file_identifier = "f142"
    topic_name = "TEST_flatbuffers_encoding"
    global_config = {'bootstrap.servers': 'localhost:9092'}

    prod = Producer(**global_config)
    buf = create_flatbuffers_object(file_identifier)
    prod.produce(topic_name, key="SIM:Spd", value=bytes(buf))
    sleep(5)

    cons = create_consumer()
    cons.subscribe([topic_name])

    msg = poll_for_valid_message(cons).value()
    file_id = msg[4:8].decode(encoding="utf-8")
    assert file_id == file_identifier
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_double(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.
    
    :param docker_compose: Test fixture
    :return: none
    """
    data_topic = "TEST_forwarderData_send_pv_update"
    pvs = ["SIM:Spd"]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()
    cons.subscribe([CONFIG_TOPIC])
    msg = poll_for_valid_message(cons)
    check_json_config(loads(str(msg.value(), encoding="utf-8")), data_topic, pvs)

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
    sleep(7)
    cons.subscribe([data_topic])

    # Poll for message which should contain forwarded PV update
    data_msg = poll_for_valid_message(cons).value()
    log_data = LogData.LogData.GetRootAsLogData(data_msg, 0)

    union_string = String.String()
    union_string.Init(log_data.Value().Bytes, log_data.Value().Pos)
    assert union_string.Value() == stop_command

    cons.close()


def check_json_config(json_object, topicname, pvs, schema="f142", channel_provider_type="ca"):
    """
    Check the json config is valid that gets sent to the configuration topic.

    :param json_object: Dictionary containing all config options
    :param topicname: The data topic name to push updates to
    :param pvs: The list of PVs to listen for changes
    :param schema: The flatbuffers schema to check against
    :param channel_provider_type: Epics v3/v4 specification
    :return: none
    """
    assert json_object["cmd"] == "add"
    configured_pvs = []
    for stream in json_object["streams"]:
        assert channel_provider_type == stream["channel_provider_type"]
        channel = stream["channel"]
        assert channel not in configured_pvs and channel in pvs
        configured_pvs.append(channel)
        converter = stream["converter"]
        assert converter["schema"] == schema
        assert converter["topic"] == topicname
