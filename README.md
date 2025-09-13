# libmqttlink

**Lightweight C/C++ MQTT client:** background connection, auto‑reconnect & re‑subscribe, per‑topic callbacks.

---

## About

`libmqttlink` is a small, **thread‑safe** MQTT client for C/C++. It maintains the broker connection in a background thread, **auto‑reconnects** on drops, **auto‑re‑subscribes** after reconnects or broker restarts, and exposes a minimal, practical API for subscribe/publish and clean shutdown. It’s designed for daemons, services, and embedded apps that need a robust MQTT link with low overhead. Licensed under **MIT**.

---

## Why libmqttlink?

Compared to using a raw MQTT C client directly, `libmqttlink` provides:

- **Lifecycle handled for you:** background worker, health monitoring, clean teardown.
- **Resilience out of the box:** automatic reconnect + automatic re‑subscribe.
- **Topic routing:** multiple topics mapped to per‑topic callbacks.
- **Sane defaults:** practical logging and a minimal surface area.
- **Thread safety:** mutex‑protected subscription and connection state.

Keep your application focused on business logic; let `libmqttlink` handle the “boring but important” parts.

---

## Features

1) **Connection Management**
- Connect using host/IP, port, username, and password.
- Background worker continuously monitors the link.
- **Automatic reconnection** on drop.
- Query the current connection state.

2) **Topic Subscription**
- Subscribe to multiple topics with per‑topic callbacks.
- Dynamic topic registration.
- **Auto re‑subscribe** after reconnects or broker restarts.

3) **Message Handling & Publishing**
- Dispatch inbound messages to user‑defined callbacks by topic.
- Publish to any topic.
- **Message storage (retain) flag** is configurable:
  - `e_libmqttlink_message_storage_flag_state_message_keep` → ask the broker to retain
  - `e_libmqttlink_message_storage_flag_state_message_dont_keep` → do not retain

> Note: The callback currently uses `const char*` for payloads (NUL‑terminated text). If you need binary payloads, consider extending the API to include a length parameter.

4) **Resource Management**
- Clean shutdown: disconnect, free memory, and destroy threads/mutexes.

5) **Logging**
- Informative logs for connection, subscription, publishing, and errors.

6) **Long‑Running Stability**
- Optional periodic connection refresh (e.g., every 24 hours) with unsubscribe/resubscribe logic.

7) **Thread Safety**
- Internal mutex protection for subscription management and connection state.

---

## Quick Start

```c
#include <libmqttlink/libmqttlink.h>

// 1) Connect to the broker
libmqttlink_establishes_connection_and_follows("host", 1883, "username", "password");

// 2) Subscribe to a topic (your callback handles messages for this topic)
void notification_callback(const char *message_contents, const char *subject) {
    // handle message here
}
libmqttlink_subscribe_subject("sensors/+/temp", notification_callback);

// 3) Publish a message
libmqttlink_send_message(
    "devices/42/cmd",
    "reboot",
    e_libmqttlink_message_storage_flag_state_message_dont_keep
);

// 4) (Optional) Query connection state
enum _enum_libmqttlink_connection_state st = libmqttlink_get_connection_state();
// st == e_libmqttlink_connection_state_connection_true ? connected : not connected

// 5) Disconnect and clean up
libmqttlink_shutdown();
```

> **Note:** After any reconnect, the library automatically restores your subscriptions.

---

## API Surface (from `include/libmqttlink.h`)

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
int  libmqttlink_establishes_connection_and_follows(
        const char *server_ip_address, int server_port,
        const char *user_name, const char *password);

void libmqttlink_shutdown(void);

int  libmqttlink_send_message(
        const char *subject, const char *message_contents,
        enum _enum_libmqttlink_message_storage_flag_state message_storage_flag_state);

int  libmqttlink_subscribe_subject(
        const char *subject,
        void (*notification_function_ptr)(const char *message_contents, const char *subject));

enum _enum_libmqttlink_connection_state libmqttlink_get_connection_state(void);
```

> **Signature authority:** Always check `include/libmqttlink.h` for the authoritative types and function signatures.

---

## Build (generic)

> Prerequisites: a C/C++ toolchain and an MQTT C client library’s development headers (plus platform‑specific threading libs where needed).

```bash
make
sudo make install
```

Then include the header in your application:
```c
#include <libmqttlink/libmqttlink.h>
```

Link against the library produced by your build plus any required system libs (e.g., thread libs), depending on your platform.

---

## License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.
