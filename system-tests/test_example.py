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

BUILD_FORWARDER = False
CONFIG_TOPIC = "TEST_forwarderConfig"
DATA_TOPIC = "TEST_forwarderData"


def test_topic_exists_on_creation(docker_compose):
    topic_suffix = "_topic_exists_on_creation"
    server = "localhost:9092"
    prod = ProducerWrapper(server, CONFIG_TOPIC + topic_suffix , DATA_TOPIC + topic_suffix)
    prod.add_config(["SIM:Spd", "SIM:ActSpd"])
    assert prod.topic_exists(CONFIG_TOPIC + topic_suffix, server)


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
    config_topic = "TEST_forwarderConfig_send_pv_update"

    consumer_config = {'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': 'smallest'},
                       'group.id': uuid.uuid4()}
    prod = ProducerWrapper("localhost:9092", config_topic , data_topic )
    prod.add_config(["SIM:Spd"])
    sleep(2)  # Waiting for config to be pushed
    cons = Consumer(**consumer_config)
    cons.subscribe([config_topic])
    msg = cons.poll()
    assert not msg.error()
    topic = msg.topic()
    # assert str(msg.value(), encoding="utf-8") == '{"cmd": "add", "streams": [{"converter": {"schema": "f142", "topic": "TEST_forwarderDatasend_pv_update"}, "channel_provider_type": "ca", "channel": "SIM:Spd"}] }'
    assert topic == config_topic      # update value
    change_pv_value("SIM:Spd", 5)
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


# def subscribe_and_poll(topic_name):
#     cons = Consumer(**{'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': 'smallest'},
#                      'group.id': uuid.uuid4()})
#     cons.subscribe([topic_name])
#     msg = cons.poll()
#     assert not msg.error()
#     cons.close()
#     return msg.value()


