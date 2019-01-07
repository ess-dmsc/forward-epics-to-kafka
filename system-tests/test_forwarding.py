from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata.Value import Value
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message
from helpers.flatbuffer_helpers import check_expected_values, check_multiple_expected_values
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE, PVSTR, PVLONG, PVENUM

CONFIG_TOPIC = "TEST_forwarderConfig"


def teardown_function(function):
    """
    Stops forwarder pv listening and resets any values in EPICS
    :param docker_compose: test fixture to apply to
    :return:
    """
    print("teardown happening", flush=True)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "")
    prod.stop_all_pvs()

    defaults = {
        PVDOUBLE: 0.0,
        # We have to use this as the second parameter for caput gets parsed as empty so does not change the value of
        # the PV
        PVSTR: "\"\"",
        PVLONG: 0,
        PVENUM: "INIT"
    }

    for key, value in defaults.items():
        change_pv_value(key, value)
    sleep(3)


def test_config_file_channel_created_correctly(docker_compose):
    """
    Test that the channel defined in the config file is created.

    :param docker_compose: Test fixture
    :return: None
    """
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "TEST_forwarderData_pv_from_config")
    cons = create_consumer()
    cons.subscribe(['TEST_forwarderData_pv_from_config'])
    sleep(5)
    # Change the PV value, so something is forwarded
    change_pv_value(PVDOUBLE, 10)
    # Wait for PV to be updated
    sleep(5)
    # Check the initial value is forwarded
    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0.0)

    # Check the new value is forwarded
    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 10.0)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_double(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    :param docker_compose: Test fixture
    :return: None
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
    check_expected_values(first_msg, Value.Double, PVDOUBLE, 0.0)

    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Double, PVDOUBLE, 5.0)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_string(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    :param docker_compose: Test fixture
    :return: None
    """

    data_topic = "TEST_forwarderData_string_pv_update"
    pvs = [PVSTR]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()
    # Update value
    change_pv_value(PVSTR, "stop")

    # Wait for PV to be updated
    sleep(5)
    cons.subscribe([data_topic])

    # Poll for empty update - initial value of PVSTR is nothing
    poll_for_valid_message(cons)
    # Poll for message which should contain forwarded PV update
    data_msg = poll_for_valid_message(cons)

    check_expected_values(data_msg, Value.String, PVSTR, b'stop')
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_long(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    NOTE: longs are converted to ints in the forwarder as they will fit in a 32 bit integer
    :param docker_compose: Test fixture
    :return: None
    """

    data_topic = "TEST_forwarderData_long_pv_update"
    pvs = [PVLONG]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(2)

    cons = create_consumer()

    # Set initial PV value
    change_pv_value(PVLONG, 0)
    sleep(2)

    # Update value
    change_pv_value(PVLONG, 5)
    # Wait for PV to be updated
    sleep(2)
    cons.subscribe([data_topic])

    first_msg = poll_for_valid_message(cons)
    check_expected_values(first_msg, Value.Int, PVLONG, 0)

    second_msg = poll_for_valid_message(cons)
    check_expected_values(second_msg, Value.Int, PVLONG, 5)
    cons.close()


def test_forwarder_sends_pv_updates_single_pv_enum(docker_compose):
    """
    Test the forwarder pushes new PV value when the value is updated.

    NOTE: Enums are converted to Ints in the forwarder.
    :param docker_compose: Test fixture
    :return: None
    """

    data_topic = "TEST_forwarderData_enum_pv_update"
    pvs = [PVENUM]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(5)

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
    cons.close()


def test_forwarder_updates_multiple_pvs(docker_compose):
    data_topic = "TEST_forwarderData_multiple"

    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(4)

    expected_values = {PVSTR: (Value.String, b''), PVLONG: (Value.Int, 0)}

    first_msg = poll_for_valid_message(cons)
    second_msg = poll_for_valid_message(cons)
    messages = [first_msg, second_msg]

    check_multiple_expected_values(messages, expected_values)
    cons.close()


def test_forwarder_updates_pv_when_config_changed_from_one_pv(docker_compose):
    data_topic = "TEST_forwarderData_change_config"
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config([PVLONG])
    prod.add_config([PVDOUBLE])

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(2)

    expected_values = {PVLONG: (Value.Int, 0), PVDOUBLE: (Value.Double, 0.0)}

    first_msg = poll_for_valid_message(cons)
    second_msg = poll_for_valid_message(cons)
    messages = [first_msg, second_msg]

    check_multiple_expected_values(messages, expected_values)
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

    expected_values = {PVSTR: (Value.String, b''), PVLONG: (Value.Int, 0), PVDOUBLE: (Value.Double, 0.0)}

    messages = [poll_for_valid_message(cons), poll_for_valid_message(cons), poll_for_valid_message(cons)]
    check_multiple_expected_values(messages, expected_values)
    cons.close()
