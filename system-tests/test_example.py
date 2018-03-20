import pytest
from kazoo.client import KazooClient


def test_can_get_info_from_zookeeper(zookeeper_container):
    zk = KazooClient(hosts=zookeeper_container)
    zk.start()
    assert(zk.get_children('/zookeeper') == ['quota'])


@pytest.mark.skip(reason="For demonstrating log output on failure")
def test__error(example_container):
    raise Exception("oh no!")
