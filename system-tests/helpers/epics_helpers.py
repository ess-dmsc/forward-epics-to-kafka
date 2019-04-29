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
    exit_code, output = container.exec_run("caput {} {}".format(pvname, value), privileged=True)
    print("caput exit code: ", exit_code, "\n")
    print("Updating PV value using caput: ")
    print(output.decode("utf-8"), "\nend\n", flush=True)

    assert exit_code == 0


def change_array_pv_value(pvname, value):
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
    exit_code, output = container.exec_run("caput -a {} {}".format(pvname, value), privileged=True)
    print("caput exit code: ", exit_code, "\n")
    print("Updating PV value using caput: ")
    print(output.decode("utf-8"), flush=True)
    assert exit_code == 0
