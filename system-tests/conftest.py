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
IOC = {"image": "screamingudder/ess-chopper-sim-ioc", "label": "chopper-sim-system-test"}
TEST_SOFTWARE = {"image": "zookeeper:3.4", "label": "zookeeper"}


def _docker_client():
    return docker.Client('unix://var/run/docker.sock', version="auto")


def pytest_runtest_logreport(report):
    if report.failed:
        docker_client = _docker_client()
        test_containers = docker_client.containers(
            all=True,
            filters={"label": TEST_SOFTWARE["label"]})
        for container in test_containers:
            log_lines = [
                ("docker inspect {!r}:".format(container['Id'])),
                (pprint.pformat(docker_client.inspect_container(container['Id']))),
                ("docker logs {!r}:".format(container['Id'])),
                (docker_client.logs(container['Id']).decode('utf-8')),
            ]
            report.longrepr.addsection('docker logs', os.linesep.join(log_lines))


def pull_image(image):
    """ Pull the specified image using docker-py
    This function will parse the result from docker-py and raise an exception
    if there is an error.
    :param image: Name of the image to pull
    """
    docker_client = _docker_client()
    response = docker_client.pull(image)
    lines = [line for line in response.splitlines() if line]

    # The last line of the response contains the overall result of the pull
    # operation.
    pull_result = json.loads(lines[-1])
    if "error" in pull_result:
        raise Exception("Could not pull {}: {}".format(
            image, pull_result["error"]))


@backoff.on_exception(backoff.expo,
                      KazooTimeoutError,
                      max_tries=10)
def wait_for_zookeeper_up(zk_address):
    zk = KazooClient(hosts=zk_address)
    zk.start()
    zk.stop()


def start_kafka(docker_client, ids):
    zk_container = start_container(docker_client, ZOOKEEPER, ids)
    wait_for_zookeeper_up(zk_container)
    kafka_env = {"KAFKA_ZOOKEEPER_CONNECT": zk_container,
                 "KAFKA_ADVERTISED_HOST_NAME": "localhost"}
    kafka_host_config = docker_client.create_host_config(port_bindings={
        9092: 9092
    })
    kafka_container = start_container(docker_client, KAFKA, ids, environment=kafka_env, host_config=kafka_host_config,
                                      ports=[9092])


def start_container(docker_client, image, ids, environment=None, host_config=None, ports=None):
    pull_image(image["image"])
    container = docker_client.create_container(
        image=image["image"],
        host_config=host_config,
        ports=ports,
        labels=[image["label"]],
        environment=environment
    )
    docker_client.start(container=container["Id"])
    container_info = docker_client.inspect_container(container.get('Id'))
    ids.append(container["Id"])

    return container_info["NetworkSettings"]["IPAddress"]


def clean_up_containers(docker_client, ids):
    for identifier in ids:
        docker_client.remove_container(
            container=identifier,
            force=True
        )


@pytest.fixture
def example_container():
    docker_client = _docker_client()
    ids = []  # keep list of contain ids to clean up later
    start_kafka(docker_client, ids)
    ioc_environment = {"FORWARDER_CONFIG_TOPIC": "TEST_forwarderConfig",
                       "FORWARDER_OUTPUT_TOPIC": "TEST_sampleEnv"}
    start_container(docker_client, IOC, ids, environment=ioc_environment)

    yield start_container(docker_client, TEST_SOFTWARE, ids)

    clean_up_containers(docker_client, ids)
