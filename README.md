# libmqttlink_mosq

A **thread-safe C/C++ MQTT client library** built on Mosquitto.  
Delivers simple, reliable APIs for connection, subscription, publishing, and clean resource management.

---

## Features

### 1) Connection Management
- Connect to any MQTT broker using host/IP, port, username, and password.
- Connection runs in a background thread and is continuously monitored.
- **Automatic reconnection** on drop.
- Query current connection state via `mqttlink_get_mosq_connection_state`.

### 2) Topic Subscription
- Subscribe to multiple topics with per-topic callback functions.
- Dynamic topic registration via `mqttlink_subscribe_mosq_subject`.
- **Auto re-subscribe** after reconnects or broker restarts.

### 3) Message Handling & Publishing
- Dispatch inbound messages to user-defined callbacks by topic.
- Publish to any topic with `mqttlink_send_mosq_message`.
- Configurable message storage flag (default **QoS 2** behavior).

### 4) Resource Management
- Clean shutdown: disconnect, free memory, and destroy threads/mutexes with `mqttlink_finished_mosq`.

### 5) Logging
- Informative logs for connection, subscription, publishing, and errors (Linux/Windows).

### 6) Automatic Broker Connection Restart
- Periodic restart every 24 hours with unsubscribe/resubscribe logic.

### 7) Thread Safety
- Internal mutex protection for subscription management and connection state.

---

## Quick Start

```c
#include <libmqttlink/libmqttlink_mosq.h>

// 1) Connect to the broker
mqttlink_establishes_mosq_connection_and_follows("host", 1883, "username", "password");

// 2) Subscribe to a topic
mqttlink_subscribe_mosq_subject("topic", notification_callback);

// 3) Publish a message
mqttlink_send_mosq_message(
    "topic",
    "message",
    e_mqttlink_mosq_message_storage_flag_state_message_dont_keep
);

// 4) Disconnect and clean up
mqttlink_finished_mosq();
```

> **Note:**  
> Your callback can branch on topic/payload as needed.  
> The library automatically restores subscriptions after reconnection.

---

## Dependencies

- [Mosquitto](https://mosquitto.org/)
- POSIX threads (`pthread`)

---

## License

This project is licensed under the MIT License.  
See the [LICENSE](LICENSE) file for details.
