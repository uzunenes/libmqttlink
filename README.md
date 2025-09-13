# libmqttlink

lightweight C/C++ MQTT client: background connection, auto-reconnect & re-subscribe, per-topic callbacks.

## Prerequisites

**You must have an MQTT C client library installed (libmosquitto).** Install it first on your system:

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

## API — Short Reference

| Function | Purpose | Returns |
|---|---|---|
| `libmqttlink_establishes_connection_and_follows(cfg*)` | Start and maintain the background connection | `int` (0 = ok) |
| `libmqttlink_subscribe_subject(topic, cb)` | Subscribe to a topic and dispatch messages to `cb` | `int` |
| `libmqttlink_send_message(topic, payload, retain)` | Publish to a topic | `int` |
| `libmqttlink_get_connection_state()` | Get connection state (online/offline) | `enum` |
| `libmqttlink_shutdown()` | Stop background worker and release resources | `void` |

> Terminology: The library names use “subject”; in this README we use **topic** consistently (same concept in MQTT).


## Errors & Logging

- Functions return `0` on success and non-zero on failure.

## Examples

See the `examples/` directory for a minimal subscriber/publisher. You can test quickly with a local broker (e.g., Mosquitto).

## License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.
