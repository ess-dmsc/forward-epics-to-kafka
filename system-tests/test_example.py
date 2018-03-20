import pytest


@pytest.mark.parametrize('docker_compose', [False], indirect=['docker_compose'])
def test_integration(docker_compose):
    pass
