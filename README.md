# Forward EPICS to Kafka

Goals are

- Forward EPICS process variables to Kafka topics
- Receive configuration from Kafka streams
- Recover from channel or topic failures

## Setup Kafka

Run Kafka broker with:
```
auto.create.topics.enable=true
delete.topic.enable=true
```

for easier experimenting.


## Installation

### Requirements

- EPICS v4
- Flatbuffers
- librdkafka
- libjansson
- C++ compiler with c++11 support (Flatbuffers requires that as well)


### Environment variables

For development, I keep these paths quite explicit in environment variables:

$flatc Directory where flatc compiler can be found.

Paths to dependencies:

$epicsbase_dir
$epicsv4_dir
$librdkafka_dir
$jansson_dir




### Build

Optional, set CC and CXX environment to your liking.

Assuming you have 'make':

```
cmake <path-to-source>
make
make docs
```

### librdkafka

```
./configure --prefix=/home/scratch/software/librdkafka/../librdkafka-install

# Passing CFLAGS, CXXFLAGS or CPPFLAGS does not work because configure prepends the custom ones!
# Luckily, the configure script shows the result..
./configure --prefix=/home/scratch/software/librdkafka/../librdkafka-install --disable-optimization
```
