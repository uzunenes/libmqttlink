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
#include <libmqttlink/libmqttlink.h>
#include <stdio.h>
#include <unistd.h>

void on_message(const char *msg, const char *topic) 
{
    printf("%s: %s\n", topic, msg);
}

int main() 
{
    libmqttlink_connect_and_monitor("127.0.0.1", 1883, "user", "pass");
    libmqttlink_subscribe_topic("a", 0, on_message);
    
    while (1) {
        libmqttlink_publish_message("a", "hello", 0);
        sleep(1);
        if(g_exit) break;
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
| `libmqttlink_publish_message(topic, payload, qos)` | Publish with QoS | `int` |
| `libmqttlink_set_will(topic, payload, qos, retain)` | Configure LWT before connect | `int` |
| `libmqttlink_set_tls(cafile, capath, certfile, keyfile, tls_version, insecure)` | Configure TLS before connect | `int` |
| `libmqttlink_get_connection_state()` | Current connection state | `enum` |
| `libmqttlink_shutdown()` | Stop worker & free resources | `void` |

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

## Troubleshooting
- Verify broker reachability (`ping`, firewall rules).
- Check credentials and ACLs.
- Use Mosquitto CLI tools for isolation: `mosquitto_sub -t a -h 127.0.0.1 -p 1883`.
- For TLS issues, run with `mosquitto_pub` first to validate certificates.

## License
MIT — see [LICENSE](LICENSE).

### Third-party dependencies

This project uses the Mosquitto C client library (libmosquitto), which is dual licensed under the Eclipse Public License 2.0 (EPL-2.0) and the Eclipse Distribution License 1.0 (EDL-1.0). See:
- https://github.com/eclipse/mosquitto/blob/master/LICENSE
- https://www.eclipse.org/legal/epl-v20.html
- https://www.eclipse.org/org/documents/edl-v10.php

If you distribute binaries or source code including Mosquitto, you must comply with the terms of these licenses.
