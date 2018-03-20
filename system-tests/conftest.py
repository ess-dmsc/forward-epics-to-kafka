import os.path
import pytest
from compose.cli.main import TopLevelCommand, project_from_options


@pytest.fixture(scope="session")
def docker_compose(request):
    """
    :type request: _pytest.python.FixtureRequest
    """
    # Allows option of preventing build occurring by decorating test function with:
    # @pytest.mark.parametrize('docker_compose', [False], indirect=['docker_compose'])
    try:
        build = request.param
    except AttributeError:
        build = True

    options = {"--no-deps": False,
               "--always-recreate-deps": False,
               "--scale": "",
               "--abort-on-container-exit": False,
               "SERVICE": "",
               "--remove-orphans": False,
               "--no-recreate": True,
               "--force-recreate": False,
               "--build": build,
               '--no-build': False,
               '--no-color': False,
               "--rmi": "none",
               "--volumes": "",
               "--follow": False,
               "--timestamps": False,
               "--tail": "all",
               "-d": True,
               "-v": True,  # remove volumes when docker-compose down (don't persist kafka and zk data)
               }

    project = project_from_options(os.path.dirname(__file__), options)
    cmd = TopLevelCommand(project)
    cmd.up(options)

    def fin():
        cmd.logs(options)
        cmd.down(options)

    # Using a finalizer rather than yield in the fixture means
    # that the containers will be brought down even if tests fail
    request.addfinalizer(fin)
