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
    kafka_container = start_container(docker_client, KAFKA, ids)


def start_container(docker_client, image, ids):
    pull_image(image["image"])
    container = docker_client.create_container(
        image=image["image"],
        labels=[image["label"]]
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
    yield start_container(docker_client, TEST_SOFTWARE, ids)

    clean_up_containers(docker_client, ids)
