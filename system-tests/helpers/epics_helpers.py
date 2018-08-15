import docker


def change_pv_value(pvname, value):
    """
    Epics call to change PV value.

    :param pvname:(string) PV name
    :param value: PV value to change to
    :return: none
    """
    container = False
    client = docker.from_env()
    for item in client.containers.list():
        if "_ioc_1" in item.name:
            container = item
            break
    if not container:
        raise Exception("IOC Container not found")
    container.exec_run("caput {} {}".format(pvname, value), privileged=True)
