#include <signal.h>
#include <stdio.h>
#include <libmqttlink/libmqttlink.h>
#include <libmqttlink/libmqttlink_utility_functions.h>

// Signal handler for graceful exit
static void exit_signal_handler(int sig);

// Callback function for incoming MQTT messages
static void message_arrived_callback(const char *message, const char *subject);

// Global variable for exit signal
static int g_exit_signal = 0;

int main(void)
{
    const char *server_ip_address = "1.1.1.1";
    int server_port = 1883;
    const char *user_name = "b";
    const char *password = "a";

    // Example topics
    const char *topic1 = "libmqttlink_test";
    const char *topic2 = "libmqttlink_test2";

    // Register signal handlers
    signal(SIGINT, exit_signal_handler);
    signal(SIGTERM, exit_signal_handler);

    // Connect to MQTT broker
    libmqttlink_establishes_connection_and_follows(server_ip_address, server_port, user_name, password);

    // Subscribe to topics
    libmqttlink_subscribe_subject(topic1, message_arrived_callback);
    libmqttlink_subscribe_subject(topic2, message_arrived_callback);

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
                libmqttlink_send_message(topic1, buffer_msg, e_libmqttlink_message_storage_flag_state_message_dont_keep);
            }
            else
            {
                sprintf(buffer_msg, "%d-test-message-topic: %s .", i, topic2);
                libmqttlink_send_message(topic2, buffer_msg, e_libmqttlink_message_storage_flag_state_message_dont_keep);
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
static void message_arrived_callback(const char *message, const char *subject)
{
    printf("%s(): Received message: [%s] - Subject: [%s]\n", __func__, message, subject);
}