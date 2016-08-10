JAVA=/usr/lib/jvm/java-1.8.0-openjdk.x86_64/bin/java
JAVA=java
#KAFKA_LIBS=/opt/kafka/libs
$JAVA -cp \
$KAFKA_LIBS/zkclient-0.8.jar:\
$KAFKA_LIBS/zookeeper-3.4.6.jar:\
$KAFKA_LIBS/kafka_2.11-0.10.0.0.jar:\
$KAFKA_LIBS/kafka-tools-0.10.0.0.jar:\
$KAFKA_LIBS/kafka-clients-0.10.0.0.jar:\
$KAFKA_LIBS/log4j-1.2.17.jar:\
$KAFKA_LIBS/slf4j-log4j12-1.7.21.jar:\
$KAFKA_LIBS/slf4j-api-1.7.21.jar:\
$KAFKA_LIBS/scala-library-2.11.8.jar:\
$KAFKA_LIBS/scala-parser-combinators_2.11-1.0.4.jar:\
. \
KafkaCreateTopics
