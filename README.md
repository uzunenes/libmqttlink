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
| `libmqttlink_connect_and_monitor(server, port, user, pass)` | Start and maintain the background connection | `int` (0 = ok) |
| `libmqttlink_subscribe_topic(topic, cb)` | Subscribe to a topic and dispatch messages to `cb` | `int` |
| `libmqttlink_publish_message(topic, payload, retain)` | Publish to a topic | `int` |
| `libmqttlink_get_connection_state()` | Get connection state (online/offline) | `enum` |
| `libmqttlink_shutdown()` | Stop background worker and release resources | `void` |

## Errors & Logging

- Functions return `0` on success and non-zero on failure.
- The library prints log messages to `stdout`/`stderr`.

## Examples

See the `examples/` directory for a minimal subscriber/publisher. You can test quickly with a local broker (e.g., Mosquitto):

```bash
./example <server_ip> <port> <username> <password>
```

## License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.
