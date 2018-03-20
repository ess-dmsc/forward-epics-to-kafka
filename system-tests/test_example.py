import pytest
from confluent_kafka import Producer, Consumer, KafkaError


@pytest.mark.parametrize('docker_compose', [False], indirect=['docker_compose'])
def test_integration(docker_compose):
    print("Produce message")
    p = Producer({'bootstrap.servers': 'localhost'})
    data = "test_message"
    p.produce('mytopic', data.encode('utf-8'))
    p.flush()

    print("Consume message")
    c = Consumer({'bootstrap.servers': 'localhost', 'group.id': 'mygroup',
                  'default.topic.config': {'auto.offset.reset': 'earliest'}})
    c.subscribe(['mytopic'])
    wait_max_ms = 2000
    msg = c.poll(wait_max_ms)
    if not msg.error():
        msg_string = msg.value().decode('utf-8')
        print('Received message: %s' % msg_string)
    elif msg.error().code() != KafkaError._PARTITION_EOF:
        print(msg.error())

    c.close()
