package org.apache.kafka.examples;

import java.io.Closeable;

import org.I0Itec.zkclient.ZkClient;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import kafka.admin.AdminOperationException;
import kafka.admin.AdminUtils;
import kafka.admin.RackAwareMode.Enforced$;
import kafka.utils.ZKStringSerializer$;
import kafka.utils.ZkUtils;

public class KafkaIncreasePartitions {

    static final Logger logger = LogManager.getLogger();

    public Test() {
        // TODO Auto-generated constructor stub
    }

    public static void addPartitions(String zkServers, String topic, int partitions) {

        try (AutoZkClient zkClient = new AutoZkClient(zkServers)) {
            ZkUtils zkUtils = ZkUtils.apply(zkClient, false);

            if (AdminUtils.topicExists(zkUtils, topic)) {
                logger.info("Altering topic {}", topic); 
                try {
                    AdminUtils.addPartitions(zkUtils, topic, partitions, "", true, Enforced$.MODULE$);
                    logger.info("Topic {} altered with partitions : {}", topic, partitions); 
                } catch (AdminOperationException aoe) {
                    logger.info("Error while altering partitions for topic : {}", topic, aoe); 
                } 
            } else {
                logger.info("Topic {} doesn't exists", topic); 
            } 
        } 
    }

    // Just exists for Closeable convenience 
    private static final class AutoZkClient extends ZkClient implements Closeable { 

        static int sessionTimeout = 30_000;
        static int connectionTimeout = 6_000;

        AutoZkClient(String zkServers) {
            super(zkServers, sessionTimeout, connectionTimeout, ZKStringSerializer$.MODULE$);
        }
    }

    public static void main(String[] args) {

        addPartitions("localhost:2181", "hello", 20);
    }
}
