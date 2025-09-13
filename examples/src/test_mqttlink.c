#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <libmqttlink/libmqttlink.h>
#include <libmqttlink/libmqttlink_utility_functions.h>

// Signal handler for graceful exit
static void exit_signal_handler(int sig);

// Callback function for incoming MQTT messages
static void message_arrived_callback(const char *message, const char *topic);

// Global variable for exit signal
static int g_exit_signal = 0;

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port> <username> <password>\n", argv[0]);
        return 1;
    }

    const char *server_ip_address = argv[1];
    int server_port = atoi(argv[2]);
    const char *user_name = argv[3];
    const char *password = argv[4];

    // Example topics
    const char *topic1 = "a";
    const char *topic2 = "b";

    // Register signal handlers
    signal(SIGINT, exit_signal_handler);
    signal(SIGTERM, exit_signal_handler);

    // Connect to MQTT broker
    libmqttlink_connect_and_monitor(server_ip_address, server_port, user_name, password);

    // Subscribe to topics
    libmqttlink_subscribe_topic(topic1, message_arrived_callback);
    libmqttlink_subscribe_topic(topic2, message_arrived_callback);

    int i = 0;
    while (1)
    {
        printf("%s(): MQTT connection status: [%d]\n", __func__, libmqttlink_get_connection_state());

        if (g_exit_signal != 0)
            break;

        if (libmqttlink_get_connection_state() == e_libmqttlink_connection_state_connection_true)
        {
            // Send a message to alternating topics
            char buffer_msg[256];
            i++;
            if (i % 2)
            {
                sprintf(buffer_msg, "%d-test-message-topic: %s .", i, topic1);
                libmqttlink_publish_message(topic1, buffer_msg, e_libmqttlink_message_storage_flag_state_message_dont_keep);
            }
            else
            {
                sprintf(buffer_msg, "%d-test-message-topic: %s .", i, topic2);
                libmqttlink_publish_message(topic2, buffer_msg, e_libmqttlink_message_storage_flag_state_message_dont_keep);
            }
        }

        libmqttlink_sleep_milisec(1000);
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
