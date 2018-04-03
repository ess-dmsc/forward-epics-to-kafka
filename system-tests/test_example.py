import pytest
from time import sleep
from helpers.producerwrapper import ProducerWrapper
from confluent_kafka import Producer, Consumer
from helpers.f142_logdata import LogData, Value, Int
import flatbuffers
import uuid
import time

BUILD_FORWARDER = False
CONFIG_TOPIC = "TEST_forwarderConfig"


@pytest.mark.parametrize('docker_compose', [BUILD_FORWARDER], indirect=['docker_compose'])
def test_topic_exists_on_creation(docker_compose):
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "TEST_forwarderData")
    prod.add_config(["Sim:Spd", "Sim:ActSpd"])
    sleep(2)
    assert prod.topic_exists(CONFIG_TOPIC)


@pytest.mark.parametrize('docker_compose', [BUILD_FORWARDER], indirect=['docker_compose'])
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
