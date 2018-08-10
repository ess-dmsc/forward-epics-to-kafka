import docker


def change_pv_value(pvname, value):
    """
    Epics call to change PV value.

    :param pvname:(string) PV name
    :param value: PV value to change to
    :return: none
    """
    client = docker.from_env()
    container = client.containers.get("forwarder_ioc_1")
    container.exec_run("caput {} {}".format(pvname, value), privileged=True)
