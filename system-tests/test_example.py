import pytest
from time import sleep
from helpers.producerwrapper import ProducerWrapper
from confluent_kafka import Producer, Consumer
from helpers.f142_logdata import LogData, Value, Int, Double
import flatbuffers
import uuid
import time
from CaChannel import CaChannel, CaChannelException
import math
from json import loads


BUILD_FORWARDER = False
CONFIG_TOPIC = "TEST_forwarderConfig"


def test_config_created_correctly(docker_compose):
    topic_suffix = "_topic_exists_on_creation"
    server = "localhost:9092"
    data_topic = "TEST_forwarderData" + topic_suffix
    pvs = ["SIM:Spd", "SIM:ActSpd"]
    prod = ProducerWrapper(server, CONFIG_TOPIC + topic_suffix, data_topic)
    prod.add_config(pvs)
    assert prod.topic_exists(CONFIG_TOPIC + topic_suffix, server)
    cons = Consumer(**{'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': 'smallest'},
                                       'group.id': uuid.uuid4()})
    cons.subscribe([CONFIG_TOPIC + topic_suffix])
    msg = cons.poll()
    assert not msg.error()
    buf_json = loads(str(msg.value(), encoding="utf-8"))
    check_json_config(buf_json, data_topic, pvs)
    cons.close()


def test_flatbuffers_encode_and_decode(docker_compose):
    global_config = {'bootstrap.servers': 'localhost:9092'}
    producer_config = global_config
    consumer_config = global_config
    consumer_config['default.topic.config'] = {'auto.offset.reset': 'smallest'}
    consumer_config['group.id'] = uuid.uuid4()
    topic_name = "TEST_flatbuffers_encoding"
    file_identifier = "f142"
    prod = Producer(**producer_config)
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
    prod.produce(topic_name, key="SIM:Spd", value=bytes(buf))
    sleep(5)
    cons = Consumer(**consumer_config)
    cons.subscribe([topic_name])
    msg = cons.poll()
    assert not msg.error()
    buffer = msg.value()
    file_id = buffer[4:8].decode(encoding="utf-8")
    assert file_id == file_identifier
    cons.close()


def test_forwarder_sends_pv_updates_single_pv(docker_compose):

    data_topic = "TEST_forwarderData_send_pv_update"
    pvs = ["SIM:Spd"]

    consumer_config = {'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': 'smallest'},
                       'group.id': uuid.uuid4()}
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    sleep(2)  # Waiting for config to be pushed
    cons = Consumer(**consumer_config)
    cons.subscribe([CONFIG_TOPIC])
    msg = cons.poll()
    assert not msg.error()
    topic = msg.topic()
    assert topic == CONFIG_TOPIC
    check_json_config(loads(str(msg.value(), encoding="utf-8")), data_topic, pvs)

    change_pv_value("SIM:Spd", 5)  # update value
    sleep(10)  # Waiting for PV to be updated
    cons.subscribe([data_topic])
    first_msg = cons.poll()
    assert not first_msg.error()
    first_msg_buf = first_msg.value()
    log_data_first = LogData.LogData.GetRootAsLogData(first_msg_buf, 0)
    check_message_value(log_data_first, Value.Value.Double, b'SIM:Spd')
    check_double_value_and_equality(log_data_first, 0)
    second_msg = cons.poll()
    assert not second_msg.error()
    second_msg_buf = second_msg.value()
    log_data_second = LogData.LogData.GetRootAsLogData(second_msg_buf, 0)
    check_message_value(log_data_second, Value.Value.Double, b'SIM:Spd')
    check_double_value_and_equality(log_data_second, 5)
    cons.close()


def check_double_value_and_equality(log_data, expected_value):
    union_double = Double.Double()
    union_double.Init(log_data.Value().Bytes, log_data.Value().Pos)
    union_value = union_double.Value()
    assert math.isclose(expected_value, union_value)


def check_message_value(log_data, value_type, value):
    assert value_type == log_data.ValueType()
    assert value == log_data.SourceName()


def change_pv_value(pvname, value):
    try:
        chan = CaChannel(pvname)
        print("created CaChannel")
        chan.searchw()
        chan.putw(value)
        chan.getw()
    except CaChannelException as e:
        print(e)


def check_json_config(json_object, topicname, pvs, schema="f142", channel_provider_type="ca"):
    assert json_object["cmd"] == "add"
    used_pvs = []
    for stream in json_object["streams"]:
        assert channel_provider_type == stream["channel_provider_type"]
        channel = stream["channel"]
        assert channel not in used_pvs and channel in pvs
        used_pvs.append(channel)
        conv = stream["converter"]
        assert conv["schema"] == schema
        assert conv["topic"] == topicname
