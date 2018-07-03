#include "../uri.h"
#include <gtest/gtest.h>

TEST(URITest, port_is_set_to_zero_on_initialisation) {
  Forwarder::URI uri1;
  ASSERT_EQ(uri1.port, 0u);
}

TEST(URITest, default_host_is_set_to_nothing_on_initialisation) {
  Forwarder::URI uri1;
  ASSERT_EQ(uri1.host, "");
}

TEST(URITest, init_with_host_port_and_path) {
  Forwarder::URI uri1("http://shizune:9000/isis_test_clusters/");
  ASSERT_EQ(uri1.port, 9000u);
  ASSERT_EQ(uri1.host, "shizune");
  ASSERT_EQ(uri1.path, "/isis_test_clusters/");
}

TEST(URITest, init_with_host_and_path) {
  Forwarder::URI uri1("http://shizune/isis_test_clusters/");
  ASSERT_EQ(uri1.host, "shizune");
  ASSERT_EQ(uri1.path, "/isis_test_clusters/");
  ASSERT_EQ(uri1.port, (uint32_t)0);
}

TEST(URITest, init_with_just_host) {
  Forwarder::URI uri1("http://shizune");
  ASSERT_EQ(uri1.host, "shizune");
  ASSERT_EQ(uri1.path, "");
  ASSERT_EQ(uri1.port, (uint32_t)0);
}

TEST(URITest, uri_sets_host_from_host_string) {
  Forwarder::URI u1("//myhost");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "myhost");
  ASSERT_EQ(u1.port, (uint32_t)0);
}

TEST(URITest, uri_sets_host_from_ip_string) {
  Forwarder::URI u1("//127.0.0.1");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "127.0.0.1");
  ASSERT_EQ(u1.port, (uint32_t)0);
}

TEST(URITest, uri_parses_port) {
  Forwarder::URI u1("//myhost:345");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "myhost");
  ASSERT_EQ(u1.port, (uint32_t)345);
}

TEST(URITest, uri_parses_port_with_no_slashes_before_or_after_host_name) {
  Forwarder::URI u1;
  u1.require_host_slashes = false;
  u1.parse("myhost:345");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "myhost");
  ASSERT_EQ(u1.port, (uint32_t)345);
}

TEST(URITest, uri_parses_port_with_no_slashes_before_or_after_ip) {
  Forwarder::URI u1("//127.0.0.1:345");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "127.0.0.1");
  ASSERT_EQ(u1.port, (uint32_t)345);
}

TEST(URITest, uri_parses_host_and_port_with_domain_seperator) {
  Forwarder::URI u1;
  u1.port = 123;
  u1.parse("http://my.host:345");
  ASSERT_EQ(u1.scheme, "http");
  ASSERT_EQ(u1.host, "my.host");
  ASSERT_EQ(u1.host_port, "my.host:345");
  ASSERT_EQ(u1.port, (uint32_t)345);
}

TEST(URITest, uri_parses_host_with_domain_seperator) {
  Forwarder::URI u1;
  u1.port = 123;
  u1.parse("http://my.host");
  ASSERT_EQ(u1.scheme, "http");
  ASSERT_EQ(u1.host, "my.host");
  ASSERT_EQ(u1.host_port, "my.host:123");
  ASSERT_EQ(u1.port, (uint32_t)123);
}

TEST(URITest,
     uri_parses_host_and_port_with_domain_seperator_followed_by_blank_path) {
  Forwarder::URI u1("kafka://my-host.com:8080/");
  ASSERT_EQ(u1.scheme, "kafka");
  ASSERT_EQ(u1.host, "my-host.com");
  ASSERT_EQ(u1.port, (uint32_t)8080);
  ASSERT_EQ(u1.path, "/");
}

TEST(
    URITest,
    uri_parses_host_and_port_with_domain_seperator_followed_by_directory_path) {
  Forwarder::URI u1("kafka://my-host.com:8080/som_e");
  ASSERT_EQ(u1.scheme, "kafka");
  ASSERT_EQ(u1.host, "my-host.com");
  ASSERT_EQ(u1.port, (uint32_t)8080);
  ASSERT_EQ(u1.path, "/som_e");
  ASSERT_EQ(u1.topic, "som_e");
}

TEST(
    URITest,
    uri_parses_host_and_port_with_domain_seperator_followed_by_multiple_directory_paths) {
  Forwarder::URI u1("kafka://my_host.com:8080/some/longer");
  ASSERT_EQ(u1.scheme, "kafka");
  ASSERT_EQ(u1.host, "my_host.com");
  ASSERT_EQ(u1.port, (uint32_t)8080);
  ASSERT_EQ(u1.path, "/some/longer");
  ASSERT_EQ(u1.topic, "");
}

TEST(URITest, uri_parses_host_without_scheme) {
  Forwarder::URI u1("//my.host/the-topic");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "my.host");
  ASSERT_EQ(u1.port, (uint32_t)0);
  ASSERT_EQ(u1.topic, "the-topic");
}

TEST(URITest, uri_parses_host_with_port_and_without_scheme) {
  Forwarder::URI u1("//my.host:789/the-topic");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "my.host");
  ASSERT_EQ(u1.port, (uint32_t)789);
  ASSERT_EQ(u1.topic, "the-topic");
}

TEST(URITest, uri_parses_absolute_path) {
  Forwarder::URI u1("/mypath/sub");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "");
  ASSERT_EQ(u1.port, (uint32_t)0);
  ASSERT_EQ(u1.path, "/mypath/sub");
  ASSERT_EQ(u1.topic, "");
}

TEST(URITest, uri_parses_relative_path) {
  Forwarder::URI u1("mypath/sub");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "");
  ASSERT_EQ(u1.port, (uint32_t)0);
  ASSERT_EQ(u1.path, "mypath/sub");
  ASSERT_EQ(u1.topic, "");
}

TEST(URITest, uri_parses_absolute_path_to_topic) {
  Forwarder::URI u1("/topic-name.test");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "");
  ASSERT_EQ(u1.port, (uint32_t)0);
  ASSERT_EQ(u1.path, "/topic-name.test");
  ASSERT_EQ(u1.topic, "topic-name.test");
}

TEST(URITest, uri_parses_relative_path_to_topic) {
  Forwarder::URI u1("topic-name.test");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "");
  ASSERT_EQ(u1.port, (uint32_t)0);
  ASSERT_EQ(u1.path, "topic-name.test");
  ASSERT_EQ(u1.topic, "topic-name.test");
}

TEST(URITest, uri_parses_host_with_domain_seperator_then_adds_path) {

  Forwarder::URI u1("/some-path");
  u1.parse("//my.host");
  ASSERT_EQ(u1.scheme, "");
  ASSERT_EQ(u1.host, "my.host");
  ASSERT_EQ(u1.port, (uint32_t)0);
  ASSERT_EQ(u1.path, "/some-path");
  ASSERT_EQ(u1.topic, "some-path");
}

TEST(URITest, trim) {
  Forwarder::URI u1("  //some:123     ");
  ASSERT_EQ(u1.host, "some");
  ASSERT_EQ(u1.port, 123u);
}
