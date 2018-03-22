import pytest
from time import sleep
from helpers.producerwrapper import ProducerWrapper

BUILD_FORWARDER = False


@pytest.mark.parametrize('docker_compose', [BUILD_FORWARDER], indirect=['docker_compose'])
def test_integration(docker_compose):
    set_up_producer("localhost:9092", "topic1", "topic2")


def set_up_producer(server, config_topic, data_topic):
    sleep(30)
    prod = ProducerWrapper(server, config_topic, data_topic)
    sleep(20)
    prod.add_config(["Sim:Spd", "Sim:ActSpd"])
    sleep(20)
    assert prod.topic_exists(data_topic)
    assert prod.topic_exists(config_topic)
