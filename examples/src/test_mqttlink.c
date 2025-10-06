#include <libmqttlink/libmqttlink.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void mqttlink_sleep_milisec(unsigned int milisec);
static void exit_signal_handler(int sig);
static void message_arrived_callback(const char *message, const char *topic);

// Global variable for exit signal
static int g_exit_signal = 0;

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port> <username> <password> [cafile] [certfile] [keyfile]\n", argv[0]);
        return 1;
    }

    const char *server_ip_address = argv[1];
    int server_port = atoi(argv[2]);
    const char *user_name = argv[3];
    const char *password = argv[4];
    const char *cafile = (argc > 5) ? argv[5] : NULL;
    const char *certfile = (argc > 6) ? argv[6] : NULL;
    const char *keyfile = (argc > 7) ? argv[7] : NULL;

    // Optional: configure Will (must be before connect)
    libmqttlink_set_will("status/lastwill", "client-offline", 1, 0);

    // Optional: configure TLS if CA or cert/key provided
    if (cafile || certfile || keyfile)
        libmqttlink_set_tls(cafile, NULL, certfile, keyfile, NULL, 0);

    // Example topics
    const char *topic1 = "a";
    const char *topic2 = "b";

    // Register signal handlers
    signal(SIGINT, exit_signal_handler);
    signal(SIGTERM, exit_signal_handler);

    // Connect to MQTT broker
    libmqttlink_connect_and_monitor(server_ip_address, server_port, user_name, password);

    // Subscribe to topics (QoS: topic1=1, topic2=0)
    if (libmqttlink_subscribe_topic(topic1, 1, message_arrived_callback) != 0)
        fprintf(stderr, "Subscribe failed for %s\n", topic1);
    if (libmqttlink_subscribe_topic(topic2, 0, message_arrived_callback) != 0)
        fprintf(stderr, "Subscribe failed for %s\n", topic2);

    int i = 0;
    while (!g_exit_signal)
    {
        enum _enum_libmqttlink_connection_state st = libmqttlink_get_connection_state();
        if (st == e_libmqttlink_connection_state_connection_true)
        {
            char buffer_msg[256];
            const int use_first = (i & 1);
            const char *topic = use_first ? topic1 : topic2;
            int qos = use_first ? 1 : 0;
            snprintf(buffer_msg, sizeof(buffer_msg), "%d-test-message-topic: %s .", ++i, topic);
            if (libmqttlink_publish_message(topic, buffer_msg, qos) != 0)
                fprintf(stderr, "Publish failed (%s)\n", topic);
            // Demonstrate dynamic unsubscribe after some messages
            if (i == 10)
            {
                libmqttlink_unsubscribe_topic(topic2);
                printf("Unsubscribed from %s\n", topic2);
            }
        }
        mqttlink_sleep_milisec(1000);
    }

    // Disconnect and cleanup
    libmqttlink_shutdown();
    printf("bye :)\n");

    return 0;
}

// Signal handler implementation
static void exit_signal_handler(int sig)
{
    g_exit_signal = sig;
}

// Callback for incoming messages
static void message_arrived_callback(const char *message, const char *topic)
{
    printf("%s(): Received message: [%s] - topic: [%s]\n", __func__, message, topic);
}

static void mqttlink_sleep_milisec(unsigned int milisec)
{
    usleep(milisec * 1000);
}