from confluent_kafka import TopicPartition
from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata.Value import Value
from time import sleep
from helpers.flatbuffer_helpers import check_expected_value, check_multiple_expected_values
from helpers.kafka_helpers import create_consumer, poll_for_valid_message, get_last_available_status_message
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE, PVSTR, PVLONG, PVENUM, PVFLOATARRAY
import json
import numpy as np

CONFIG_TOPIC = "TEST_forwarderConfig"
INITIAL_FLOATARRAY_VALUE = (1.1, 2.2, 3.3)


def teardown_function(function):
    """
    Stops forwarder pv listening and resets any values in EPICS
    """
    print("Resetting PVs", flush=True)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, "")
    prod.stop_all_pvs()

    defaults = {
        PVDOUBLE: 0.0,
        # We have to use this as the second parameter for caput gets parsed as empty so does not change the value of
        # the PV
        PVSTR: "",
        PVLONG: 0,
        PVENUM: np.array(["INIT"]).astype(np.string_)
    }

    for key, value in defaults.items():
        change_pv_value(key, value)
    change_pv_value(PVFLOATARRAY, INITIAL_FLOATARRAY_VALUE)
    sleep(3)


def test_forwarder_sends_pv_updates_single_pv_enum(docker_compose_no_command):
    """
    GIVEN PV of enum type is configured to be forwarded
    WHEN PV value is updated
    THEN Forwarder publishes the update to Kafka

    NOTE: Enums are converted to Ints in the forwarder.
    """
    data_topic = "TEST_forwarderData_enum_pv_update"
    pvs = [PVENUM]

    sleep(5)
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(5)

    cons = create_consumer()

    # Update value
    change_pv_value(PVENUM, np.array(["START"]).astype(np.string_))
    # Wait for PV to be updated
    cons.subscribe([data_topic])
    sleep(5)

    first_msg, _ = poll_for_valid_message(cons)
    check_expected_value(first_msg, Value.Int, PVENUM, 0)

    second_msg, _ = poll_for_valid_message(cons)
    check_expected_value(second_msg, Value.Int, PVENUM, 1)
    cons.close()


def test_forwarder_sends_pv_updates_single_floatarray(docker_compose_no_command):
    """
    GIVEN PV of enum type is configured to be forwarded
    WHEN PV value is updated
    THEN Forwarder publishes the update to Kafka
    """

    data_topic = "TEST_forwarderData_floatarray_pv_update"
    pvs = [PVFLOATARRAY]

    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    # Wait for config to be pushed
    sleep(5)

    cons = create_consumer()

    # Wait for PV to be updated
    cons.subscribe([data_topic])
    sleep(5)

    first_msg, _ = poll_for_valid_message(cons)
    check_expected_value(first_msg, Value.ArrayFloat, PVFLOATARRAY, INITIAL_FLOATARRAY_VALUE)
    cons.close()


def test_forwarder_updates_multiple_pvs(docker_compose_no_command):
    """
    GIVEN multiple PVs (string and long types) are configured to be forwarded
    WHEN PV value is updated
    THEN Forwarder publishes the updates to Kafka
    """
    data_topic = "TEST_forwarderData_multiple"

    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(4)

    expected_values = {PVSTR: (Value.String, b""), PVLONG: (Value.Int, 0)}

    first_msg, _ = poll_for_valid_message(cons)
    second_msg, _ = poll_for_valid_message(cons)
    messages = [first_msg, second_msg]

    check_multiple_expected_values(messages, expected_values)
    cons.close()


def test_forwarder_status_shows_added_pvs(docker_compose_no_command):
    """
    GIVEN A PV (double type) is already being forwarded
    WHEN A message configures two additional PV (str and long types) to be forwarded
    THEN Forwarder status message lists new PVs
    """
    data_topic = "TEST_forwarderData_change_config"
    status_topic = "TEST_forwarderStatus"
    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)

    sleep(5)
    cons = create_consumer()
    sleep(2)
    cons.assign([TopicPartition(status_topic, partition=0)])
    sleep(2)

    # Get the last available status message
    partitions = cons.assignment()
    _, hi = cons.get_watermark_offsets(partitions[0], cached=False, timeout=2.0)
    last_msg_offset = hi - 1
    cons.assign([TopicPartition(status_topic, partition=0, offset=last_msg_offset)])
    status_msg, _ = poll_for_valid_message(cons, expected_file_identifier=None)

    status_json = json.loads(status_msg)
    names_of_channels_being_forwarded = {stream['channel_name'] for stream in status_json['streams']}
    expected_names_of_channels_being_forwarded = {PVSTR, PVLONG}

    assert expected_names_of_channels_being_forwarded == names_of_channels_being_forwarded, \
        f"Expect these channels to be configured as forwarded: {expected_names_of_channels_being_forwarded}, " \
            f"but status message report these as forwarded: {names_of_channels_being_forwarded}"

    cons.close()


def test_forwarder_updates_pv_when_config_change_add_two_pvs(docker_compose_no_command):
    """
    GIVEN A PV (double type) is already being forwarded
    WHEN A message configures two additional PV (str and long types) to be forwarded
    THEN Forwarder publishes initial values for added PVs
    """
    data_topic = "TEST_forwarderData_change_config"
    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)

    sleep(2)
    cons = create_consumer()
    sleep(2)
    cons.subscribe([data_topic])
    sleep(2)

    poll_for_valid_message(cons)
    poll_for_valid_message(cons)

    expected_values = {PVSTR: (Value.String, b""), PVLONG: (Value.Int, 0)}

    messages = [poll_for_valid_message(cons)[0], poll_for_valid_message(cons)[0]]
    check_multiple_expected_values(messages, expected_values)
    cons.close()


def test_forwarder_can_handle_rapid_config_updates(docker_compose_no_command):
    status_topic = "TEST_forwarderStatus"
    data_topic = "TEST_forwarderData_connection_status"

    base_pv = PVDOUBLE
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    configured_list_of_pvs = []
    number_of_config_updates = 100
    for i in range(number_of_config_updates):
        pv = base_pv + str(i)
        prod.add_config([pv])
        configured_list_of_pvs.append(pv)

    sleep(5)
    cons = create_consumer()
    sleep(2)
    cons.assign([TopicPartition(status_topic, partition=0)])
    sleep(2)
    # Get the last available status message
    status_msg = get_last_available_status_message(cons, status_topic)

    streams_json = json.loads(status_msg)['streams']
    streams = []
    for item in streams_json:
        streams.append(item['channel_name'])

    for pv in configured_list_of_pvs:
        assert pv in streams, "Expect configured PV to be reported as being forwarded"
