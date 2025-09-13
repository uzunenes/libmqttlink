# libmqttlink

Lightweight C/C++ MQTT client: background connection, auto-reconnect & re-subscribe, per-topic callbacks.

## Prerequisites

You need the Mosquitto C client library installed:

```bash
sudo apt update
sudo apt install -y libmosquitto-dev
```

## Build & Install

```bash
git clone https://github.com/uzunenes/libmqttlink
cd libmqttlink
make
sudo make install
```

## Usage (quick start)

Run the example client with your broker info:

```bash
./example <server_ip> <port> <username> <password>
# e.g.
./example 127.0.0.1 1883 myuser mypass
```

It connects in the background, auto-reconnects, re-subscribes, and prints any messages it receives on the demo topics (`a`, `b`). It also publishes a test message every second alternating between those topics.

## Minimal code example

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <libmqttlink/libmqttlink.h>
#include <libmqttlink/libmqttlink_utility_functions.h>

static volatile int g_exit = 0;
static void on_signal(int s){ (void)s; g_exit = 1; }
static void on_message(const char *msg, const char *topic){
    printf("message on '%s': %s\n", topic, msg);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <username> <password>\n", argv[0]);
        return 1;
    }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    libmqttlink_connect_and_monitor(argv[1], atoi(argv[2]), argv[3], argv[4]);
    libmqttlink_subscribe_topic("a", on_message);
    libmqttlink_subscribe_topic("b", on_message);

    int i = 0;
    while (!g_exit) {
        if (libmqttlink_get_connection_state() == e_libmqttlink_connection_state_connection_true) {
            char buf[128];
            sprintf(buf, "%d-test-message-topic: %s .", ++i, (i % 2) ? "a" : "b");
            libmqttlink_publish_message((i % 2) ? "a" : "b", buf,
                e_libmqttlink_message_storage_flag_state_message_dont_keep);
        }
        libmqttlink_sleep_milisec(1000);
    }
    libmqttlink_shutdown();
    return 0;
}
```

## API — Short reference

| Function | Purpose | Returns |
|---|---|---|
| `libmqttlink_connect_and_monitor(server, port, user, pass)` | Start and maintain background connection | `int` (0 = ok) |
| `libmqttlink_subscribe_topic(topic, cb)` | Subscribe and dispatch messages to `cb` | `int` |
| `libmqttlink_publish_message(topic, payload, retain)` | Publish to a topic | `int` |
| `libmqttlink_get_connection_state()` | Get connection state (online/offline) | `enum` |
| `libmqttlink_shutdown()` | Stop worker and free resources | `void` |

## Errors & Logging

- Functions return `0` on success and non‑zero on failure.
- The library prints helpful logs to stdout/stderr.

## Troubleshooting

- Verify broker reachability (`ping`, firewall rules).
- Check credentials and ACLs.
- Try a local test with Mosquitto tools: `mosquitto_sub -t a -h 127.0.0.1 -p 1883`.
- If nothing prints, ensure the broker allows your client and topics.

## License

MIT — see [LICENSE](LICENSE).
