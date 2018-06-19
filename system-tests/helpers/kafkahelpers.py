from confluent_kafka import Consumer
import uuid


def poll_for_valid_message(consumer):
    """
    Polls the subscribed topics by the consumer and checks the buffer is not empty or malformed.

    :param consumer: The consumer object.
    :return: The message object received from polling.
    """
    msg = consumer.poll()
    assert not msg.error()
    return msg


def create_consumer():
    consumer_config = {'bootstrap.servers': 'localhost:9092', 'default.topic.config': {'auto.offset.reset': 'smallest'},
                       'group.id': uuid.uuid4()}
    cons = Consumer(**consumer_config)
    return cons
