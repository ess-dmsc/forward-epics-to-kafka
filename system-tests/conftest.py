import json
import pprint
import os
import docker
import pytest
import backoff
from kazoo.client import KazooClient
from kazoo.handlers.threading import KazooTimeoutError

ZOOKEEPER = {"image": "zookeeper:3.4", "label": "zookeeper-system-test"}
KAFKA = {"image": "wurstmeister/kafka:0.11.0.1", "label": "kafka-system-test"}
IOC = {"image": "screamingudder/ess-chopper-sim-ioc:latest", "label": "chopper-sim-system-test"}
TEST_SOFTWARE = {"image": "zookeeper:3.4", "label": "zookeeper"}


def _docker_client():
    return docker.from_env()


def pytest_runtest_logreport(report):
    if report.failed:
        docker_client = _docker_client()
        test_containers = docker_client.api.containers(
            all=True,
            filters={"label": TEST_SOFTWARE["label"]})
        for container in test_containers:
            log_lines = [
                ("docker inspect {!r}:".format(container['Id'])),
                (pprint.pformat(docker_client.api.inspect_container(container['Id']))),
                ("docker logs {!r}:".format(container['Id'])),
                (docker_client.api.logs(container['Id']).decode('utf-8')),
            ]
            report.longrepr.addsection('docker logs', os.linesep.join(log_lines))


@backoff.on_exception(backoff.expo,
                      KazooTimeoutError,
                      max_tries=10)
def wait_for_zookeeper_up(zk_address):
    zk = KazooClient(hosts=zk_address)
    zk.start()
    zk.stop()


def start_kafka(docker_client, containers):
    zk_container = start_container(docker_client, ZOOKEEPER)
    wait_for_zookeeper_up(get_ip(docker_client, zk_container))
    kafka_env = {"KAFKA_ZOOKEEPER_CONNECT": get_ip(docker_client, zk_container),
                 "KAFKA_ADVERTISED_HOST_NAME": "localhost"}
    port_bindings = {"9092": 9092}
    kafka_container = start_container(docker_client, KAFKA, environment=kafka_env, ports=port_bindings)
    containers.extend([zk_container, kafka_container])


def start_container(docker_client, image_info, environment=None, ports=None):
    image = docker_client.images.pull(image_info["image"])
    container = docker_client.containers.run(
        image=image,
        ports=ports,
        labels=[image_info["label"]],
        environment=environment,
        detach=True
    )
    return container


def clean_up_containers(containers):
    for container in containers:
        container.stop()
        container.remove()


def get_ip(docker_client, container):
    return docker_client.api.inspect_container(container.id)["NetworkSettings"]["IPAddress"]


@pytest.fixture
def example_container():
    docker_client = _docker_client()
    containers = []  # keep list of containers to clean up later
    start_kafka(docker_client, containers)
    ioc_environment = {"FORWARDER_CONFIG_TOPIC": "TEST_forwarderConfig",
                       "FORWARDER_OUTPUT_TOPIC": "TEST_sampleEnv"}
    ioc_container = start_container(docker_client, IOC, environment=ioc_environment)
    containers.append(ioc_container)

    #forwarder = docker_client.images.build(fileobj="../Dockerfile")

    test_container = start_container(docker_client, TEST_SOFTWARE)
    containers.append(test_container)
    yield get_ip(docker_client, test_container)

    clean_up_containers(containers)
