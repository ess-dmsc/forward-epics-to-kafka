from helpers.producerwrapper import ProducerWrapper
from helpers.f142_logdata.Value import Value
from helpers.ep00 import EventType
from time import sleep
from helpers.kafka_helpers import create_consumer, poll_for_valid_message, get_all_available_messages, \
    poll_for_connection_status_message
from helpers.flatbuffer_helpers import check_expected_values, check_multiple_expected_values, \
    check_expected_connection_status_values
from helpers.epics_helpers import change_pv_value
from helpers.PVs import PVDOUBLE, PVSTR, PVLONG, PVENUM
from confluent_kafka import TopicPartition
import json

CONFIG_TOPIC = "TEST_forwarderConfig"


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
        PVSTR: "\"\"",
        PVLONG: 0,
        PVENUM: "INIT"
    }

    for key, value in defaults.items():
        change_pv_value(key, value)
    sleep(3)


def test_config_file_channel_created_correctly(docker_compose):
    """
    GIVEN Forwarder is started with a config file specifying a PV to forward (forwarder_config.json)
    WHEN PV value is updated
    THEN Forwarder publishes the update to Kafka

    :param docker_compose: Test fixture (see https://docs.pytest.org/en/latest/fixture.html)
    """
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


def test_connection_status_messages(docker_compose):
    """
      GIVEN PV is configured to be forwarded
      WHEN Connection status changes
      THEN Forwarder publishes ep00 message with connection status

      NOTE: Enums are converted to Ints in the forwarder.
      """
    data_topic = "TEST_forwarderData_connection_status"
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

    first_msg = poll_for_connection_status_message(cons)
    check_expected_connection_status_values(first_msg, EventType.EventType.CONNECTED)

    cons.close()


def test_forwarder_sends_pv_updates_single_pv_enum(docker_compose):
    """
    GIVEN PV of enum type is configured to be forwarded
    WHEN PV value is updated
    THEN Forwarder publishes the update to Kafka

    NOTE: Enums are converted to Ints in the forwarder.
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

    expected_values = {PVSTR: (Value.String, b''), PVLONG: (Value.Int, 0)}

    first_msg = poll_for_valid_message(cons)
    second_msg = poll_for_valid_message(cons)
    messages = [first_msg, second_msg]

    check_multiple_expected_values(messages, expected_values)
    cons.close()


def test_forwarder_updates_pv_when_config_changed_from_two_pvs_to_three(docker_compose):
    """
    GIVEN A message configures two PVs (str and long types) to be forwarded
    WHEN A message configures an additional PV (double type) to be forwarded
    THEN Forwarder publishes value for all PVs
    """
    data_topic = "TEST_forwarderData_change_config"
    status_topic = "TEST_forwarderStatus"
    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)
    sleep(2)
    prod.add_config([PVDOUBLE])

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
    status_msg = poll_for_valid_message(cons, expected_file_identifier=None)

    status_json = json.loads(status_msg)
    names_of_channels_being_forwarded = {stream['channel_name'] for stream in status_json['streams']}
    expected_names_of_channels_being_forwarded = {PVDOUBLE, PVSTR, PVLONG}

    assert expected_names_of_channels_being_forwarded == names_of_channels_being_forwarded, \
        f"Expect these channels to be configured as forwarded: {expected_names_of_channels_being_forwarded}, " \
            f"but status message report these as forwarded: {names_of_channels_being_forwarded}"

    cons.close()


def test_updates_from_the_same_pv_reach_the_same_partition(docker_compose):
    """
    GIVEN Topic to publish data to has multiple partitions and multiple PVs are configured to be forwarded
    WHEN PVs values change
    THEN All PV updates for a particular PV are published to the same partition

    We want updates for a particular PV to all reach the same partition in Kafka
    so that their order is maintained
    By default the messages would be published to different partitions
    as a round robin approach is used to balance load, and this test would fail
    But we have used the PV name as the message key, which should ensure
    all messages from the same PV end up in the same partition
    """
    # This topic was created in the docker-compose and has 2 partitions
    data_topic = "TEST_forwarderData_2_partitions"
    pvs = [PVSTR, PVLONG]
    prod = ProducerWrapper("localhost:9092", CONFIG_TOPIC, data_topic)
    prod.add_config(pvs)

    consumer_partition_0 = create_consumer()
    consumer_partition_0.assign([TopicPartition(data_topic, partition=0)])

    consumer_partition_1 = create_consumer()
    consumer_partition_1.assign([TopicPartition(data_topic, partition=1)])

    sleep(5)

    # There should be 3 messages in total for each PV
    # (the initial value and these two updates(not counting connection status messages))
    change_pv_value(PVSTR, "1")
    sleep(0.5)
    change_pv_value(PVSTR, "2")
    sleep(0.5)

    change_pv_value(PVLONG, 1)
    sleep(0.5)
    change_pv_value(PVLONG, 2)
    sleep(0.5)

    sleep(5)

    def test_all_messages_for_pv_are_in_one_partition(consumer):
        messages = get_all_available_messages(consumer)
        # if we have any messages in this partition
        if len(messages) != 0:
            pv_name = messages[0].key()
            assert pv_name is not None
            count_of_messages_with_pv_name = sum(1 for message in messages if message.key() == pv_name
                                                 and message.value()[4:8] != b'ep00')
            # then we expect exactly 4 with the same key as the first message
            assert count_of_messages_with_pv_name == 3
            return True
        return False

    # Must find three messages with the same key in at least one of the two partitions
    assert test_all_messages_for_pv_are_in_one_partition(consumer_partition_0) or \
           test_all_messages_for_pv_are_in_one_partition(consumer_partition_1)
