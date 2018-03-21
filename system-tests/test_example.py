import pytest
from time import sleep
from helpers.producer import Producer

BUILD_FORWARDER = False


@pytest.mark.parametrize('docker_compose', [BUILD_FORWARDER], indirect=['docker_compose'])
def test_integration(docker_compose):
    set_up_producer("localhost:9092", "topic1", "topic2")


def set_up_producer(server, config_topic, data_topic):
    sleep(10)
    prod = Producer(server, config_topic, data_topic)
    sleep(10)
    assert prod.topic_exists(data_topic)
    assert prod.topic_exists(config_topic)
