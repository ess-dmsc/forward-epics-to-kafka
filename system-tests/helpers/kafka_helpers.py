from confluent_kafka import Consumer
import uuid
from helpers.f142_logdata import LogData


def poll_for_valid_message(consumer):
    """
    Polls the subscribed topics by the consumer and checks the buffer is not empty or malformed.

    :param consumer: The consumer object.
    :return: The message object received from polling.
    """
    msg = consumer.poll()
    if msg.error():
        print("Consumer error: {}".format(msg.error()))
    assert not msg.error()
    return LogData.LogData.GetRootAsLogData(msg.value(), 0)


def create_consumer(offset_reset="earliest"):
    consumer_config = {'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': offset_reset},
                       'group.id': uuid.uuid4()}
    cons = Consumer(**consumer_config)
    return cons
