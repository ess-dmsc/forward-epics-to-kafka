from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata.Value import Value
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.flatbuffer_helpers import check_expected_values
from helpers.epics_helpers import change_pv_value
from PVs import PVDOUBLE, PVSTR, PVLONG, PVENUM

CONFIG_TOPIC = "TEST_forwarderConfig"


def test_config_file_channel_created_correctly(docker_compose):
    """
    Test that the channel defined in the config file is created.

    :param docker_compose: Test fixture
    :return: none
    """

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "TEST_forwarderData_pv_from_config")
    cons = create_consumer()
    cons.subscribe(['TEST_forwarderData_pv_from_config'])
    # Change the PV value, so something is forwarded
    change_pv_value(PVDOUBLE, 10)
    # Wait for PV to be updated
    sleep(5)
    # Check the initial value is forwarded
    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0)

    # Check the new value is forwarded
    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 10)

    change_pv_value(PVDOUBLE, 0)
    prod.stop_all()
    sleep(5)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_double(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    :param docker_compose: Test fixture
    :return: none
    """

    data_topic = "TEST_forwarderData_double_pv_update"
    pvs = [PVDOUBLE]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()
    cons.subscribe([data_topic])
    # Update value
    change_pv_value(PVDOUBLE, 5)
    # Wait for PV to be updated
    sleep(5)

    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0)

    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 5)

    change_pv_value(PVDOUBLE, 0)
    prod.stop_all()
    sleep(3)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_string(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    :param docker_compose: Test fixture
    :return: none
    """

    data_topic = "TEST_forwarderData_string_pv_update"
    pvs = [PVSTR]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()
    # Update value
    stop_command = b'stop'
    change_pv_value(PVSTR, stop_command)

    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    # Poll for empty update - initial value of SIMPLE:STR is nothing
    poll_for_valid_message(cons)
    # Poll for message which should contain forwarded PV update
    data_msg = poll_for_valid_message(cons)

    check_expected_values(data_msg, Value.String, PVSTR, stop_command)

    change_pv_value(PVSTR, "")
    prod.stop_all()
    sleep(3)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_long(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    NOTE: longs are converted to ints in the forwarder as they will fit in a 32 bit integer
    :param docker_compose: Test fixture
    :return: none
    """

    data_topic = "TEST_forwarderData_long_pv_update"
    pvs = [PVLONG]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()

    # Update value
    change_pv_value(PVLONG, 5)
    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Int, PVLONG, 0)

    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Int, PVLONG, 5)

    change_pv_value(PVLONG, 0)
    prod.stop_all()
    sleep(3)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_enum(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    NOTE: longs are converted to ints in the forwarder as they will fit in a 32 bit integer
    :param docker_compose: Test fixture
    :return: none
    """

    data_topic = "TEST_forwarderData_enum_pv_update"
    pvs = [PVENUM]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()

    # Update value
    change_pv_value(PVENUM, "START")
    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Int, PVENUM, 0)

    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Int, PVENUM, 1)

    change_pv_value(PVENUM, "INIT")
    prod.stop_all()
    sleep(3)
    cons.close()


def test_forwarder_updates_multiple_pvs(docker_compose):
    data_topic = "TEST_forwarderData_multiple_pv_different"

    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(2)

    first_msg = poll_for_valid_message(cons).value()
    check_expected_values(first_msg, Value.String, PVSTR)

    second_msg = poll_for_valid_message(cons).value()
    check_expected_values(second_msg, Value.Int, PVLONG)
    cons.close()


def test_forwarder_updates_pv_when_config_changed_from_one_pv(docker_compose):
    data_topic = "TEST_forwarderData_change_config"
    pvs = [PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    prod.add_config([PVDOUBLE])

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(2)

    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Int, PVLONG)

    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE)
    cons.close()


def test_forwarder_updates_pv_when_config_changed_from_two_pvs(docker_compose):
    data_topic = "TEST_forwarderData_change_config"
    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    sleep(2)
    prod.add_config([PVDOUBLE])

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(2)

    poll_for_valid_message(cons)
    poll_for_valid_message(cons)
    poll_for_valid_message(cons)
    poll_for_valid_message(cons)

    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE)
