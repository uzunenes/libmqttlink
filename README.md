# libmqttlink

**Lightweight C/C++ MQTT client:** background connection, auto‑reconnect & re‑subscribe, per‑topic callbacks.

---

## About

`libmqttlink` is a small, **thread‑safe** MQTT client for C/C++. It maintains a broker connection in a background thread, **auto‑reconnects** on drops, **auto‑re‑subscribes** after reconnects or broker restarts, and exposes minimal, practical APIs for subscribe/publish and clean shutdown. Designed for daemons, services, and embedded apps that need a robust MQTT link with low overhead. Licensed under **MIT**.

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
- Configurable message storage flag (default **QoS 2**‑style behavior).

### 4) Resource Management
- Clean shutdown: disconnect, free memory, and destroy threads/mutexes.

### 5) Logging
- Informative logs for connection, subscription, publishing, and errors (Linux/Windows).

### 6) Long‑Running Stability
- Optional periodic connection refresh (e.g., every 24 hours) with unsubscribe/resubscribe logic.

### 7) Thread Safety
- Internal mutex protection for subscription management and connection state.

---

## Quick Start

```c
#include <libmqttlink/libmqttlink.h>

// 1) Connect to the broker
libmqttlink_connect("host", 1883, "username", "password");

// 2) Subscribe to a topic (your callback handles messages for this topic)
void notification_callback(const char *message_contents, const char *subject) {
    // handle message here
}
libmqttlink_subscribe("topic", notification_callback);

// 3) Publish a message
libmqttlink_publish(
    "topic",
    "message",
    e_libmqttlink_message_storage_flag_state_message_dont_keep
);

// 4) (Optional) Query connection state
// enum _enum_libmqttlink_connection_state st = libmqttlink_get_connection_state();

// 5) Disconnect and clean up
libmqttlink_shutdown();
```

> **Note:** Your callback can branch on topic/payload as needed. Subscriptions are automatically restored after reconnection.

---

## API Surface (at a glance)

```c
// Enums
enum _enum_libmqttlink_connection_state {
    e_libmqttlink_connection_state_connection_false,
    e_libmqttlink_connection_state_connection_true
};

enum _enum_libmqttlink_message_storage_flag_state {
    e_libmqttlink_message_storage_flag_state_message_dont_keep,
    e_libmqttlink_message_storage_flag_state_message_keep
};

// Core API
int  libmqttlink_connect(const char* host, int port, const char* user, const char* pass);
int  libmqttlink_subscribe(const char* subject, void (*notification_cb)(const char* message_contents, const char* subject));
int  libmqttlink_publish(const char* subject, const char* message_contents, enum _enum_libmqttlink_message_storage_flag_state flag);
int  libmqttlink_get_connection_state(void);
int  libmqttlink_shutdown(void);
```

> **Signature authority:** Check `libmqttlink.h` in the repo for the authoritative types and function signatures.

---

## Build (generic)

> Prerequisites: C/C++ toolchain, a build system (e.g., CMake/Make), and MQTT client dev headers (e.g., Mosquitto).

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

And link your application against the produced library plus any required system libs (e.g., pthread, MQTT client lib).

---

## Notes & Recommendations

- **QoS:** Default behavior aligns with **QoS 2** unless configured otherwise.  
- **Resilience:** Subscriptions are automatically restored after reconnects/broker restarts.  
- **Threading:** Subscription/connection state is protected by internal mutexes.  
- **Long‑running services:** A 24h periodic connection refresh helps keep links healthy.

---

## License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.

---

## GitHub About (one‑liner)

> **Lightweight C/C++ MQTT client:** background connection, auto‑reconnect & re‑subscribe, per‑topic callbacks.
