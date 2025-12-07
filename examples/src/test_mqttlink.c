#include <libmqttlink/libmqttlink.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void sleep_milisec(unsigned int milisec);
static void exit_signal_handler(int sig);
static void on_message_received(const char *message, const char *topic);

static int g_exit_signal = 0;

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port> <username> <password> [cafile] [certfile] [keyfile]\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char *username = argv[3];
    const char *password = argv[4];
    const char *cafile = (argc > 5) ? argv[5] : NULL;
    const char *certfile = (argc > 6) ? argv[6] : NULL;
    const char *keyfile = (argc > 7) ? argv[7] : NULL;

    // broker publishes this message if connection drops (must be set before connecting)
    libmqttlink_set_will("status/lastwill", "client-offline", 1, 0);

    // TLS settings (if certificate is provided)
    if (cafile || certfile || keyfile)
        libmqttlink_set_tls(cafile, NULL, certfile, keyfile, NULL, 0);

    // topics to subscribe
    const char *topic1 = "sensor/temperature";
    const char *topic2 = "sensor/humidity";

    // signal handler for ctrl+c exit
    signal(SIGINT, exit_signal_handler);
    signal(SIGTERM, exit_signal_handler);

    // connect to broker
    libmqttlink_connect_and_monitor(server_ip, server_port, username, password);

    // subscribe to topics
    if (libmqttlink_subscribe_topic(topic1, 1, on_message_received) != 0)
        fprintf(stderr, "Subscribe failed: %s\n", topic1);
    if (libmqttlink_subscribe_topic(topic2, 0, on_message_received) != 0)
        fprintf(stderr, "Subscribe failed: %s\n", topic2);

    int counter = 0;
    while (!g_exit_signal)
    {
        enum _enum_libmqttlink_connection_state state = libmqttlink_get_connection_state();
        if (state == e_libmqttlink_connection_state_connection_true)
        {
            char msg[256];
            const int use_first = (counter & 1);
            const char *topic = use_first ? topic1 : topic2;
            int qos = use_first ? 1 : 0;
            snprintf(msg, sizeof(msg), "test message #%d - topic: %s", ++counter, topic);

            if (libmqttlink_publish_message(topic, msg, qos) != 0)
            {
                fprintf(stderr, "Failed to send message: %s\n", topic);
            }

            // unsubscribe from second topic after 10 messages
            if (counter == 10)
            {
                libmqttlink_unsubscribe_topic(topic2);
                printf("Unsubscribed from %s\n", topic2);
            }
        }

        sleep_milisec(1000);
    }

    // close connection and cleanup
    libmqttlink_shutdown();
    printf("Program finished\n");

    return 0;
}

static void exit_signal_handler(int sig)
{
    g_exit_signal = sig;
}

static void on_message_received(const char *message, const char *topic)
{
    printf("Message received - Topic: [%s] Content: [%s]\n", topic, message);
}

static void sleep_milisec(unsigned int milisec)
{
    usleep(milisec * 1000);
}