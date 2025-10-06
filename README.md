# libmqttlink

Lightweight C/C++ MQTT client: background connection, auto-reconnect (exponential backoff), automatic (re)subscribe with per‑topic callbacks, optional TLS and Last Will.

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
./test_mqttlink <server_ip> <port> <username> <password> [cafile] [certfile] [keyfile]
# e.g.
./test_mqttlink 127.0.0.1 1883 myuser mypass
```

It connects in the background, auto‑reconnects with exponential backoff, re‑subscribes, and prints any messages it receives on the demo topics (`a`, `b`). It also publishes a test message every second alternating between those topics. After 10 messages the example dynamically unsubscribes from topic `b`.

## Minimal code example (basic)

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <libmqttlink/libmqttlink.h>

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

    /* Optional: configure Last Will & TLS BEFORE connect */
    libmqttlink_set_will("status/lastwill", "client-offline", 1, 0);          // qos=1 retain=0
    /* libmqttlink_set_tls(cafile, NULL, certfile, keyfile, NULL, 0); */         // example if you have certs

    if (libmqttlink_connect_and_monitor(argv[1], atoi(argv[2]), argv[3], argv[4]) != 0) {
        fprintf(stderr, "connect failed\n");
        return 1;
    }

    /* Subscribe with explicit QoS */
    libmqttlink_subscribe_topic("a", 1, on_message); // QoS1
    libmqttlink_subscribe_topic("b", 0, on_message); // QoS0

    int i = 0;
    while (!g_exit) {
        if (libmqttlink_get_connection_state() == e_libmqttlink_connection_state_connection_true) {
            const char *topic = (i % 2) ? "a" : "b";
            int qos = (i % 2) ? 1 : 0;
            char buf[128];
            snprintf(buf, sizeof(buf), "%d-test-message-topic: %s .", ++i, topic);
            libmqttlink_publish_message(topic, buf, qos,
                e_libmqttlink_message_storage_flag_state_message_dont_keep);
            if (i == 10) {
                libmqttlink_unsubscribe_topic("b");
                printf("Unsubscribed from b\n");
            }
        }
        libmqttlink_sleep_milisec(1000);
    }
    libmqttlink_shutdown();
    return 0;
}
```

## TLS + Will (quick snippet)
```c
libmqttlink_set_will("clients/123/status", "offline", 1, 1); // retain last status
libmqttlink_set_tls("/etc/ssl/certs/ca.pem", NULL,
                    "/etc/ssl/certs/client.crt", "/etc/ssl/private/client.key",
                    NULL, 0); // last 0 => verify server cert
libmqttlink_connect_and_monitor(host, port, user, pass);
```

## API — Short reference

| Function | Purpose | Returns |
|----------|---------|---------|
| `libmqttlink_connect_and_monitor(server, port, user, pass)` | Start background thread, connect & maintain session | `int` (0 = ok) |
| `libmqttlink_subscribe_topic(topic, qos, cb)` | Subscribe with QoS (0/1/2) and register callback | `int` |
| `libmqttlink_unsubscribe_topic(topic)` | Remove subscription (will not auto re-subscribe) | `int` |
| `libmqttlink_publish_message(topic, payload, qos, retainFlagEnum)` | Publish with QoS & retain flag enum | `int` |
| `libmqttlink_set_will(topic, payload, qos, retain)` | Configure LWT before connect | `int` |
| `libmqttlink_set_tls(cafile, capath, certfile, keyfile, tls_version, insecure)` | Configure TLS before connect | `int` |
| `libmqttlink_get_connection_state()` | Current connection state | `enum` |
| `libmqttlink_shutdown()` | Stop worker & free resources | `void` |
| `libmqttlink_sleep_milisec(ms)` | Sleep helper | `void` |
| `libmqttlink_get_system_time()` | Seconds (double) since epoch | `double` |
| `libmqttlink_get_current_system_time_and_date(buf)` | Timestamp string (microsecond) | `int` |
| `libmqttlink_get_primary_IP(buf)` | Primary IPv4 address | `int` |

### QoS & Retain
- QoS is per call for publish and stored per subscription.
- Retain is controlled by the enum `e_libmqttlink_message_storage_flag_state_message_keep` or `_dont_keep`.

### Reconnect behavior
- Exponential backoff starting at 500 ms, doubling up to 30 s.
- Automatic re-subscribe of all active topics after reconnection.
- Periodic (24h) forced reconnect currently hard‑coded (future: configurable).

## Errors & Logging

- Functions return `0` on success and negative (`-1`) on failure.
- Library currently logs to stdout using `printf` (planned: pluggable log callback & levels).

## Roadmap (next)
- Configurable reconnect & forced restart policy
- Logging abstraction & error codes
- Multi-instance (remove global singleton)
- Metrics/statistics API
- Optional offline publish queue

## Troubleshooting
- Verify broker reachability (`ping`, firewall rules).
- Check credentials and ACLs.
- Use Mosquitto CLI tools for isolation: `mosquitto_sub -t a -h 127.0.0.1 -p 1883`.
- For TLS issues, run with `mosquitto_pub` first to validate certificates.

## License

MIT — see [LICENSE](LICENSE).
