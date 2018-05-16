from time import sleep
from helpers.producerwrapper import ProducerWrapper
from confluent_kafka import Producer, Consumer
from helpers.f142_logdata import LogData, Value, Int, Double, String
import flatbuffers
import uuid
import time
from epics import caput, caget
import math
from json import loads


BUILD_FORWARDER = False
CONFIG_TOPIC = "TEST_forwarderConfig"


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


def create_flatbuffers_object(file_identifier):
    """
    Create a sample flatbuffers buffer.
    
    :param file_identifier: The flatbuffers schema ID
    :return: The constructed buffer
    """
    builder = flatbuffers.Builder(512)
    source_name = builder.CreateString("test")
    Int.IntStart(builder)
    Int.IntAddValue(builder, 2)
    int1 = Int.IntEnd(builder)
    LogData.LogDataStart(builder)
    LogData.LogDataAddSourceName(builder, source_name)
    LogData.LogDataAddValueType(builder, Value.Value().Int)
    LogData.LogDataAddValue(builder, int1)
    LogData.LogDataAddTimestamp(builder, int(time.time()))
    end_offset = LogData.LogDataEnd(builder)
    builder.Finish(end_offset)
    buf = builder.Output()
    buf[4:8] = bytes(file_identifier, encoding="utf-8")
    return buf


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
    cons.subscribe([CONFIG_TOPIC])
    config_msg = poll_for_valid_message(cons)

    # Update value
    stop_command = "stop"
    change_pv_value(write_PV_name, stop_command)
    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    # Poll for message which should contain forwarded PV update
    data_msg = poll_for_valid_message(cons).value()
    log_data = LogData.LogData.GetRootAsLogData(data_msg, 0)
    check_message_pv_name_and_value_type(log_data, Value.Value.String, read_PV_name.encode('utf-8'))
    union_string = String.String()
    union_string.Init(log_data.Value().Bytes, log_data.Value().Pos)
    union_value = union_string.Value()
    # Check expected PV update did occur
    assert stop_command == caget(read_PV_name)
    # Check PV update was forwarded
    assert stop_command == union_value.decode('utf8')

    cons.close()


def poll_for_valid_message(consumer):
    """
    Polls the subscribed topics by the consumer and checks the buffer is not empty or malformed.
    
    :param consumer: The consumer object.
    :return: The message object received from polling.
    """
    msg = consumer.poll()
    assert not msg.error()
    return msg


def create_consumer():
    consumer_config = {'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': 'smallest'},
                       'group.id': uuid.uuid4()}
    cons = Consumer(**consumer_config)
    return cons


def check_double_value_and_equality(log_data, expected_value):
    """
    Initialises the log data object from bytes and checks the union table
    and converts to Python Double then compares against the expected Double value.
    
    :param log_data: Log data object from the received stream buffer
    :param expected_value: Double value to compare against
    :return: none
    """
    union_double = Double.Double()
    union_double.Init(log_data.Value().Bytes, log_data.Value().Pos)
    union_value = union_double.Value()
    assert math.isclose(expected_value, union_value)


def check_message_pv_name_and_value_type(log_data, value_type, pv_name):
    """
    Checks the message name (PV) and value type (type of PV).
    
    :param log_data: Log data object from the received stream buffer
    :param value_type: Flatbuffers value type
    :param pv_name: Byte encoded string of the PV/channel name
    :return: none
    """
    assert value_type == log_data.ValueType()
    assert pv_name == log_data.SourceName()


def change_pv_value(pvname, value):
    """
    Epics call to change PV value.

    :param pvname:(string) PV name
    :param value: PV value to change to
    :return: none
    """
    caput(pvname, value, wait=True)


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
