# libmqttlink

A lightweight MQTT client library written in C. It handles connection management in the background, automatically reconnects when the connection drops, and restores subscriptions. Supports TLS and Last Will.

Version: 1.0.1

## Requirements

Mosquitto C library must be installed:

```bash
sudo apt update
sudo apt install -y libmosquitto-dev
```

## Build and Install

```bash
git clone https://github.com/uzunenes/libmqttlink
cd libmqttlink
make
sudo make install
```

## Example Usage

To build and run the example application:

```bash
cd examples
make
./test_mqttlink 127.0.0.1 1883 username password
```

For TLS:

```bash
./test_mqttlink 127.0.0.1 8883 username password /path/ca.pem /path/client.crt /path/client.key
```

The example application subscribes to two topics (a and b). It publishes a message to these topics every second. After 10 messages it unsubscribes from topic b.

## Basic Code Example

```c
#include <libmqttlink/libmqttlink.h>
#include <stdio.h>
#include <unistd.h>

void on_message(const char *message, const char *topic) 
{
    printf("Topic: %s, Message: %s\n", topic, message);
}

int main() 
{
    // connect
    libmqttlink_connect_and_monitor("127.0.0.1", 1883, "username", "password");
    
    // subscribe to topic
    libmqttlink_subscribe_topic("sensor/temperature", 1, on_message);
    
    // publish messages
    while (1) 
    {
        libmqttlink_publish_message("sensor/temperature", "25.5", 1);
        sleep(5);
    }

    libmqttlink_shutdown();
    return 0;
}
```

## TLS and Last Will Usage

TLS and Will settings must be configured before connecting:

```c
// broker publishes this message if connection drops unexpectedly
libmqttlink_set_will("device/status", "offline", 1, 1);

// TLS settings
libmqttlink_set_tls("/etc/ssl/ca.pem", NULL, "/etc/ssl/client.crt", "/etc/ssl/client.key", NULL, 0);

// now connect
libmqttlink_connect_and_monitor("broker.example.com", 8883, "username", "password");
```

## Functions

libmqttlink_connect_and_monitor: Connects to the broker and monitors connection state in the background. Automatically reconnects if connection drops. Returns 0 on success, -1 on error.

libmqttlink_subscribe_topic: Subscribes to a topic. Takes QoS value (0, 1 or 2) and a callback function to be called when a message arrives.

libmqttlink_unsubscribe_topic: Unsubscribes from a topic.

libmqttlink_publish_message: Publishes a message. Takes topic, message content and QoS value.

libmqttlink_set_will: Sets the Last Will message. Broker publishes this message if connection drops abnormally.

libmqttlink_set_tls: Configures TLS certificate settings.

libmqttlink_get_connection_state: Returns current connection state.

libmqttlink_shutdown: Closes the connection and cleans up resources.

## Reconnection Behavior

When connection drops, the library automatically tries to reconnect. It waits 500ms on the first attempt, doubling the wait time after each failed attempt. Maximum wait time is 30 seconds. All subscriptions are automatically restored when connection is established.

Connection is refreshed every 24 hours.

## Error Handling

All functions return 0 on success, -1 on error. The library writes error messages to stdout.

## Troubleshooting

If connection fails:
- Check the broker address and port
- Verify username and password are correct
- Check firewall settings
- If using TLS, verify certificate paths are correct

For testing, you can use mosquitto command line tools:

```bash
mosquitto_sub -t test -h 127.0.0.1 -p 1883 -u username -P password
mosquitto_pub -t test -m "test" -h 127.0.0.1 -p 1883 -u username -P password
```

## License

Distributed under the MIT license. See the LICENSE file for details.

## Dependencies

This project uses the Mosquitto C client library (libmosquitto). Mosquitto is distributed under Eclipse Public License 2.0 and Eclipse Distribution License 1.0.
