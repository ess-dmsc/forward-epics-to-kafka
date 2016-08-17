// First commandline argument is the host:   host:port  e.g. localhost:2181

import java.util.List;
import java.util.ArrayList;
import java.util.Properties;
import kafka.admin.AdminUtils;
import kafka.utils.ZKStringSerializer$;
import kafka.utils.ZkUtils;
import org.I0Itec.zkclient.ZkClient;
import org.I0Itec.zkclient.ZkConnection;
import kafka.admin.RackAwareMode;

public class KafkaCreateTopics {
	public static void main(String[] args) throws Exception {
		ZkClient zkClient = null;
		ZkUtils zkUtils = null;
		try {
			// Comma separated list
			String zookeeperHosts = "localhost:2181";
			if (args.length >= 1) zookeeperHosts = args[0];
			System.out.println("Connect to host: " + zookeeperHosts);
			int sessionTimeOutInMs = 15 * 1000; // 15 secs
			int connectionTimeOutInMs = 10 * 1000; // 10 secs

			zkClient = new ZkClient(zookeeperHosts, sessionTimeOutInMs, connectionTimeOutInMs, ZKStringSerializer$.MODULE$);
			zkUtils = new ZkUtils(zkClient, new ZkConnection(zookeeperHosts), false);

			int noOfPartitions = 1;
			int noOfReplication = 1;
			Properties topicConfiguration = new Properties();

			List<String> topics = new ArrayList<String>();
			topics.add("configuration.global");
			for (int i1 = 0; i1 < 32; ++i1) {
				String topicName = String.format("pv.%06d", i1);
				topics.add(topicName);
			}

			for (String topic_name : topics) {
				try {
					AdminUtils.createTopic(zkUtils, topic_name, noOfPartitions, noOfReplication, topicConfiguration, RackAwareMode.Enforced$.MODULE$);
				}
				catch (kafka.common.TopicExistsException e) {
					System.out.println("Exception: " + e);
				}
			}

		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		finally {
			if (zkClient != null) {
				zkClient.close();
			}
		}
	}
}
