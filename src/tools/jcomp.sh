# NOTE
# KAFKA_LIBS must be set such that libraries are found.

javac -cp \
$KAFKA_LIBS/zkclient-0.8.jar:\
$KAFKA_LIBS/zookeeper-3.4.6.jar:\
$KAFKA_LIBS/kafka_2.11-0.10.0.0.jar:\
$KAFKA_LIBS/kafka-tools-0.10.0.0.jar:\
$KAFKA_LIBS/kafka-clients-0.10.0.0.jar \
KafkaCreateTopics.java
