from .forwarderconfig import ForwarderConfig
from kafka import KafkaProducer, errors, SimpleClient


class Producer:
    """
    A wrapper class for the kafka producer.
    """

    def __init__(self, server, config_topic, data_topic):
        self.topic = config_topic
        self.converter = ForwarderConfig(data_topic)
        self.set_up_producer(server)

    def set_up_producer(self, server):
        try:
            self.client = SimpleClient(server)
            self.producer = KafkaProducer(bootstrap_servers=server)
            if not self.topic_exists(self.topic):
                print("WARNING: topic {} does not exist. It will be created by default.".format(self.topic))
        except errors.NoBrokersAvailable:
            print("No brokers found on server: " + server[0])
            quit()
        except errors.ConnectionError:
            print("No server found, connection error")
            quit()
        except errors.InvalidConfigurationError:
            print("Invalid configuration")
            quit()
        except errors.InvalidTopicError:
            print("Invalid topic, to enable auto creation of topics set"
                          " auto.create.topics.enable to false in broker configuration")
            quit()

    def add_config(self, pvs):
        """
        Creates a forwarder configuration to add more pvs to be monitored.

        Args:
             pvs (list): A list of new PVs to add to the forwarder configuration.

        Returns:
            None.
        """

        data = self.converter.create_forwarder_configuration(pvs)
        print("Sending data {}".format(data))
        self.producer.send(self.topic, bytes(data))

    def topic_exists(self, topicname):
        return topicname in self.client.topics

    def remove_config(self, pvs):
        """
        Creates a forwarder configuration to remove pvs that are being monitored.

        Args:
            pvs (list): A list of PVs to remove from the forwarder configuration.

        Returns:
            None.
        """

        data = self.converter.remove_forwarder_configuration(pvs)
        for pv in data:
            print("Sending data {}".format(data))
            self.producer.send(self.topic, bytes(pv))
