import json
import pprint
import os
import docker
from docker import APIClient
import pytest
import backoff
from kazoo.client import KazooClient
from kazoo.handlers.threading import KazooTimeoutError

ZOOKEEPER = {"image": "zookeeper:3.4", "label": "zookeeper-system-test"}
KAFKA = {"image": "wurstmeister/kafka:0.11.0.1", "label": "kafka-system-test"}
IOC = {"image": "dmscid/lewis:latest", "label": "chopper-sim-system-test"}
FORWARDER_LABEL = "forwarder-system-test"


def _docker_client():
    return docker.from_env()


def pytest_runtest_logreport(report):
    if report.failed:
        docker_client = _docker_client()
        test_containers = docker_client.api.containers(
            all=True,
            filters={"label": FORWARDER_LABEL})
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
    return get_ip(docker_client, kafka_container)


def start_container(docker_client, image_info, environment=None, ports=None, command=None):
    image = docker_client.images.pull(image_info["image"])
    container = docker_client.containers.run(
        image=image,
        command=command,
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
def zookeeper_container():
    docker_client = _docker_client()
    zk_container = start_container(docker_client, ZOOKEEPER)
    yield get_ip(docker_client, zk_container)
    clean_up_containers([zk_container])


@pytest.fixture
def example_container():
    docker_client = _docker_client()
    containers = []  # keep list of containers to clean up later
    kafka_address = start_kafka(docker_client, containers)
    ioc_command = "chopper -p \"epics: {prefix: 'SIM:'}\""
    ioc_container = start_container(docker_client, IOC, command=ioc_command)
    containers.append(ioc_container)

    print("before build")

    # The "low-level API" (APIClient) seems to be recommended for builds
    api_client = APIClient('unix://var/run/docker.sock', version="auto")
    tag_string = "forwarder-test"
    for response_line in api_client.build(path="../", dockerfile="Dockerfile", tag=tag_string):
        json_reponse = json.loads(response_line.decode("utf-8"))
        if 'stream' in json_reponse:
            print(json_reponse['stream'], flush=True)
        else:
            print(json_reponse, flush=True)

    forwarder_image = docker_client.images.get(tag_string)

    forwarder_env = {"BROKER": kafka_address,
                     "STATUS_URI": "//" + kafka_address + ":9092/TEST_forwarderConfig"}
    forwarder_container = docker_client.containers.run(
        image=forwarder_image,
        labels=[FORWARDER_LABEL],
        environment=forwarder_env,
        detach=True
    )
    containers.append(forwarder_container)

    print("after build and run")

    yield get_ip(docker_client, ioc_container)

    clean_up_containers(containers)
