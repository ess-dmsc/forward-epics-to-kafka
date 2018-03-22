import pytest
from time import sleep
from helpers.producerwrapper import ProducerWrapper
from helpers.LogData import *
from confluent_kafka import Producer, Consumer
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
    TOPIC_NAME = "TEST_flatbuffers_encoding"
    conf = {'bootstrap.servers': 'localhost:9092', 'group.id': uuid.uuid4()}
    prod = Producer(**conf)
    builder = flatbuffers.Builder(512)
    LogDataStart(builder)
    LogDataAddSourceName(builder, bytes("Test"), encoding='utf-8')
    LogDataAddValueType(builder, bytes("Int"), encoding='utf-8')
    LogDataAddValue(builder, 2)
    LogDataAddTimestamp(builder, time.time())
    data = LogDataEnd(builder)
    prod.produce(TOPIC_NAME, key=data, value="")
    sleep(5)
    cons = Consumer(**conf)
    cons.subscribe(TOPIC_NAME)
    msg = cons.poll
    assert msg