# libmqttlink

**Lightweight C/C++ MQTT client:** background connection, auto‑reconnect & re‑subscribe, per‑topic callbacks.  
*Powered by Mosquitto internally (for now). This repo uses the unified name **libmqttlink** while exported symbols currently carry a `_mosq` suffix.*

---

## About

`libmqttlink` is a small, **thread‑safe** MQTT client for C/C++. It keeps your broker connection alive in a background thread, **auto‑reconnects** on drops, **auto‑re‑subscribes** after reconnects or broker restarts, and exposes simple APIs for subscribe/publish and clean shutdown.  
The initial implementation is built on **Mosquitto**. Public naming has been unified under `libmqttlink`; function symbols presently include `_mosq` to reflect the backend and may be generalized in a future release.

---

## Features

### 1) Connection Management
- Connect to any MQTT broker using host/IP, port, username, and password.
- Connection runs in a background thread and is continuously monitored.
- **Automatic reconnection** on drop.
- Query the current connection state.

### 2) Topic Subscription
- Subscribe to multiple topics with per‑topic callback functions.
- Dynamic topic registration.
- **Auto re‑subscribe** after reconnects or broker restarts.

### 3) Message Handling & Publishing
- Dispatch inbound messages to user‑defined callbacks by topic.
- Publish to any topic.
- Configurable message storage flag (default **QoS 2** behavior).

### 4) Resource Management
- Clean shutdown: disconnect, free memory, and destroy threads/mutexes.

### 5) Logging
- Informative logs for connection, subscription, publishing, and errors (Linux/Windows).

### 6) Automatic Connection Refresh
- Optional periodic restart (e.g., every 24 hours) with unsubscribe/resubscribe logic.

### 7) Thread Safety
- Internal mutex protection for subscription management and connection state.

---

## Quick Start

```c
#include <libmqttlink/libmqttlink.h>

// 1) Connect to the broker
libmqttlink_establishes_mosq_connection_and_follows("host", 1883, "username", "password");

// 2) Subscribe to a topic (your callback handles messages for this topic)
void notification_callback(const char *message_contents, const char *subject) {
    // handle message here
}
libmqttlink_subscribe_mosq_subject("topic", notification_callback);

// 3) Publish a message
libmqttlink_send_mosq_message(
    "topic",
    "message",
    e_libmqttlink_message_storage_flag_state_message_dont_keep
);

// 4) Query connection state (optional)
enum _enum_libmqttlink_connection_state st = libmqttlink_get_mosq_connection_state();
/* st == e_libmqttlink_connection_state_connection_true ? connected : not connected */

// 5) Disconnect and clean up
libmqttlink_finished_mosq();
```

> **Note:** Your callback can branch on topic/payload as needed. Subscriptions are automatically restored after reconnection.

---

## API Reference (as of current headers)

```c
// Connection state
enum _enum_libmqttlink_connection_state {
    e_libmqttlink_connection_state_connection_false,
    e_libmqttlink_connection_state_connection_true
};

// Message storage (retain) flag
enum _enum_libmqttlink_message_storage_flag_state {
    e_libmqttlink_message_storage_flag_state_message_dont_keep,
    e_libmqttlink_message_storage_flag_state_message_keep
};

// Establish and monitor the connection
int libmqttlink_establishes_mosq_connection_and_follows(
    const char *server_ip_address, int server_port, const char *user_name, const char *password);

// Close connection and free resources
void libmqttlink_finished_mosq(void);

// Publish a message
int libmqttlink_send_mosq_message(
    const char *subject, const char *message_contents,
    enum _enum_libmqttlink_message_storage_flag_state message_storage_flag_state);

// Subscribe to a topic with a callback
int libmqttlink_subscribe_mosq_subject(
    const char *subject,
    void (*notification_function_ptr)(const char *message_contents, const char *subject));

// Get current connection state
enum _enum_libmqttlink_connection_state libmqttlink_get_mosq_connection_state(void);
```

> **Signature authority:** Always check `libmqttlink.h` for the source‑of‑truth signatures.

---

## Build (generic)

> Prerequisites: C/C++ toolchain, a build system (e.g., CMake/Make), and **Mosquitto** development headers.

```bash
# Example (Debian/Ubuntu)
# sudo apt-get update && sudo apt-get install -y libmosquitto-dev

# Generic out-of-tree build (if you use CMake)
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Then include the header:
```c
#include <libmqttlink/libmqttlink.h>
```

And link your application against the produced library plus Mosquitto/pthread (platform dependent).

---

## Notes & Recommendations

- **QoS:** Default behavior matches **QoS 2** unless configured otherwise.  
- **Resilience:** Subscriptions are automatically restored after reconnects/broker restarts.  
- **Threading:** Subscription/connection state is protected by internal mutexes.  
- **Long‑running services:** A 24h periodic connection refresh helps keep links healthy.

---

## Dependencies

- **Mosquitto** (client library)
- POSIX threads (`pthread`)

---

## License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.

---

## GitHub About (repo tagline)

> **Lightweight C/C++ MQTT client:** background connection, auto‑reconnect & re‑subscribe, per‑topic callbacks. (Powered by Mosquitto.)
