from confluent_kafka import Consumer
import uuid
from helpers.f142_logdata import LogData


class MsgErrorException(Exception):
    pass


def get_all_available_messages(consumer):
    """
    Consumes all available messages topics subscribed to by the consumer
    :param consumer: The consumer object
    :return: list of messages, empty if none available
    """
    messages = []
    low_offset, high_offset = consumer.get_watermark_offsets(consumer.assignment()[0], cached=False)
    number_of_messages_available = high_offset - low_offset
    while len(messages) < number_of_messages_available:
        message = consumer.poll(timeout=2.0)
        if message is None or message.error():
            continue
        messages.append(message)
    return messages


def poll_for_valid_message(consumer, expected_file_identifier=b"f142"):
    """
    Polls the subscribed topics by the consumer and checks the buffer is not empty or malformed.

    :param consumer: The consumer object
    :param expected_file_identifier: The schema id we expect to find in the message
    :return: The LogData flatbuffer from the message payload
    """
    msg = consumer.poll(timeout=1.0)
    assert msg is not None
    if msg.error():
        raise MsgErrorException("Consumer error when polling: {}".format(msg.error()))
    message_file_id = msg.value()[4:8]
    assert (expected_file_identifier == message_file_id), \
        f"Expected message to have schema id of {expected_file_identifier}, but it has {message_file_id}"
    return LogData.LogData.GetRootAsLogData(msg.value(), 0)


def create_consumer(offset_reset="earliest"):
    consumer_config = {'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': offset_reset},
                       'group.id': uuid.uuid4()}
    cons = Consumer(**consumer_config)
    return cons
