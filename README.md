# libmqttlink_mosq

C/C++ MQTT client library using Mosquitto.  
Provides simple APIs for connection, subscription, publishing, and resource management.

---

## Features

### 1. MQTT Connection Management
- Connect to an MQTT broker with IP, port, username, and password.
- Connection runs in a background thread and is monitored automatically.
- Automatic reconnection if connection drops.
- Connection state query (`mqttlink_get_mosq_connection_state`).

### 2. Topic Subscription
- Subscribe to multiple topics with custom callback functions.
- Dynamic topic registration (`mqttlink_subscribe_mosq_subject`).
- Automatic (re)subscription after reconnect or broker restart.

### 3. Message Handling
- Receive messages and dispatch to user-defined callbacks based on topic.
- Publish messages to any topic (`mqttlink_send_mosq_message`).
- Configurable message storage flag (QoS 2 by default).

### 4. Resource Management
- Clean shutdown: disconnect, free memory, destroy threads and mutexes (`mqttlink_finished_mosq`).

### 5. Logging
- Informative logging for connection, subscription, publishing, and errors (Linux/Windows).

### 6. Automatic Broker Connection Restart
- Periodic broker connection restart (every 24 hours) with unsubscribe/resubscribe logic.

### 7. Thread Safety
- Internal mutex protection for subscription management and connection state.

---

## Basic API Usage

```c
#include <libmqttlink/libmqttlink_mosq.h>

// Connect to broker
mqttlink_establishes_mosq_connection_and_follows("host", 1883, "username", "password");

// Subscribe to topic
mqttlink_subscribe_mosq_subject("topic", notification_callback);

// Publish message
mqttlink_send_mosq_message("topic", "message", e_mqttlink_mosq_message_storage_flag_state_message_dont_keep);

// Disconnect and cleanup
mqttlink_finished_mosq();
```

---

## Dependencies

- Mosquitto
- pthread

---

## License

MIT