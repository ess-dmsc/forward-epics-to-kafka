import pytest
from time import sleep
from helpers.producerwrapper import ProducerWrapper

BUILD_FORWARDER = False
CONFIG_TOPIC = "TEST_forwarderConfig"


@pytest.mark.parametrize('docker_compose', [BUILD_FORWARDER], indirect=['docker_compose'])
def test_integration(docker_compose):
    set_up_producer("localhost:9092", CONFIG_TOPIC, "TEST_forwarderData")


def set_up_producer(server, config_topic, data_topic):
    prod = ProducerWrapper(server, config_topic, data_topic)
    prod.add_config(["Sim:Spd", "Sim:ActSpd"])
    sleep(2)
    assert prod.topic_exists(config_topic)
