import os.path
import pytest
from compose.cli.main import TopLevelCommand, project_from_options
from confluent_kafka import Producer


def wait_until_kafka_ready(docker_cmd, docker_options):
    print('Waiting for Kafka broker to be ready for system tests...')
    conf = {'bootstrap.servers': 'localhost:9092',
            'api.version.request': True}
    producer = Producer(**conf)
    kafka_ready = False

    def delivery_callback(err, msg):
        nonlocal n_polls
        nonlocal kafka_ready
        if not err:
            print('Kafka is ready!')
            kafka_ready = True

    n_polls = 0
    while n_polls < 10 and not kafka_ready:
        producer.produce('waitUntilUp', value='Test message', on_delivery=delivery_callback)
        producer.poll(10)
        n_polls += 1

    if not kafka_ready:
        docker_cmd.down(docker_options)  # Bring down containers cleanly
        raise Exception('Kafka broker was not ready after 100 seconds, aborting tests.')


common_options = {"--no-deps": False,
                  "--always-recreate-deps": False,
                  "--scale": "",
                  "--abort-on-container-exit": False,
                  "SERVICE": "",
                  "--remove-orphans": False,
                  "--no-recreate": True,
                  "--force-recreate": False,
                  '--no-build': False,
                  '--no-color': False,
                  "--rmi": "none",
                  "--volumes": True,  # Remove volumes when docker-compose down (don't persist kafka and zk data)
                  "--follow": False,
                  "--timestamps": False,
                  "--tail": "all",
                  "--detach": True
                  }


@pytest.fixture(scope="module")
def docker_compose(request):
    """
    :type request: _pytest.python.FixtureRequest
    """
    print("Started preparing test environment...", flush=True)
    build = False

    # Options must be given as long form
    options = common_options
    options["--build"] = build
    options["--file"] = ["docker-compose.yml"]

    project = project_from_options(os.path.dirname(__file__), options)
    cmd = TopLevelCommand(project)
    print("Running docker-compose up", flush=True)
    cmd.up(options)
    print("\nFinished docker-compose up\n", flush=True)
    wait_until_kafka_ready(cmd, options)

    def fin():
        cmd.logs(options)
        # Stop the containers then remove them and their volumes (--volumes option)
        cmd.down(options)

    # Using a finalizer rather than yield in the fixture means
    # that the containers will be brought down even if tests fail
    request.addfinalizer(fin)


@pytest.fixture(scope="module")
def docker_compose_fake_epics(request):
    """
    :type request: _pytest.python.FixtureRequest
    """
    print("Started preparing test environment...", flush=True)
    build = False

    # Options must be given as long form
    options = common_options
    options["--build"] = build
    options["--file"] = ["docker-compose-fake-epics.yml"]

    project = project_from_options(os.path.dirname(__file__), options)
    cmd = TopLevelCommand(project)
    print("Running docker-compose up", flush=True)
    cmd.up(options)
    print("\nFinished docker-compose up\n", flush=True)
    wait_until_kafka_ready(cmd, options)

    def fin():
        cmd.logs(options)
        # Stop the containers then remove them and their volumes (--volumes option)
        cmd.down(options)

    # Using a finalizer rather than yield in the fixture means
    # that the containers will be brought down even if tests fail
    request.addfinalizer(fin)


@pytest.fixture(scope="module")
def docker_compose_idle_updates(request):
    """
    :type request: _pytest.python.FixtureRequest
    """
    print("Started preparing test environment...", flush=True)
    build = False

    # Options must be given as long form
    options = common_options
    options["--build"] = build
    options["--file"] = ["docker-compose-idle-updates.yml"]

    project = project_from_options(os.path.dirname(__file__), options)
    cmd = TopLevelCommand(project)
    print("Running docker-compose up", flush=True)
    cmd.up(options)
    print("\nFinished docker-compose up\n", flush=True)
    wait_until_kafka_ready(cmd, options)

    def fin():
        cmd.logs(options)
        # Stop the containers then remove them and their volumes (--volumes option)
        cmd.down(options)

    # Using a finalizer rather than yield in the fixture means
    # that the containers will be brought down even if tests fail
    request.addfinalizer(fin)
