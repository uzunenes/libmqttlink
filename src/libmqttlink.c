#include "../include/libmqttlink.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <mosquitto.h>
#include <net/if.h>
#include <netdb.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x02
#endif

static int get_mac_address(char *mac_str, size_t len)
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        return -1;

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET && !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            snprintf(mac_str, len, "%02x%02x%02x%02x%02x%02x", s->sll_addr[0], s->sll_addr[1], s->sll_addr[2], s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
            freeifaddrs(ifaddr);
            return 0;
        }
    }
    freeifaddrs(ifaddr);
    return -1;
}

static void generate_client_id(char *id, size_t len)
{
    char mac[32] = {0};
    get_mac_address(mac, sizeof(mac));

    time_t now = time(NULL);
    int pid = getpid();

    if (mac[0] != '\0')
        snprintf(id, len, "libmqttlink-%s-%ld-%d", mac, now, pid);
    else
        snprintf(id, len, "libmqttlink-%ld-%d", now, pid);
}

static void mqttlink_sleep_milisec(unsigned int milisec)
{
    usleep(milisec * 1000);
}

static double mqttlink_get_system_time(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

// Structure for notification callback and topic
struct struct_notification_structer
{
    void (*notification_function_ptr)(const char *message_contents, const char *topic);
    char topic[1024];
    int qos; // added per-topic QoS
    pthread_t thread_id;
};

// Main MQTT link structure
struct struct_libmqttlink_struct
{
    struct mosquitto *mosquitto_structer_ptr;
    struct struct_notification_structer *notification_structer_ptr;
    uint8_t number_of_notification_structer;
    const char *server_ip_address;
    uint16_t server_port;
    const char *user_name;
    const char *password;
    pthread_t link_control_thread_id;
    enum _enum_libmqttlink_connection_state connection_state_flag;
    // Will configuration
    const char *will_topic;
    const char *will_payload;
    int will_qos;
    int will_retain;
    // TLS configuration
    const char *tls_cafile;
    const char *tls_capath;
    const char *tls_certfile;
    const char *tls_keyfile;
    const char *tls_version;
    int tls_insecure;
};

// Global variables
static struct struct_libmqttlink_struct g_libmqttlink_struct = {
    .mosquitto_structer_ptr = NULL,
    .notification_structer_ptr = NULL,
    .number_of_notification_structer = 0,
    .server_ip_address = NULL,
    .server_port = 0,
    .user_name = NULL,
    .password = NULL,
    .link_control_thread_id = 0,
    .connection_state_flag = e_libmqttlink_connection_state_connection_false,
    .will_topic = NULL,
    .will_payload = NULL,
    .will_qos = 0,
    .will_retain = 0,
    .tls_cafile = NULL,
    .tls_capath = NULL,
    .tls_certfile = NULL,
    .tls_keyfile = NULL,
    .tls_version = NULL,
    .tls_insecure = 0,
};

static pthread_mutex_t g_mutex_lock;
static pthread_mutex_t g_state_mutex; // protects connection_state_flag
static volatile bool subsc_fonk_check_flag = 0;
static volatile bool g_stop_flag = false; // graceful stop flag

// Internal: Dispatch received messages to the correct callback (thread-safe)
static void message_received_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    (void)mosq;
    (void)obj;
    void (*cb)(const char *, const char *) = NULL;
    // lock while searching list
    pthread_mutex_lock(&g_mutex_lock);
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    uint8_t n = ptr->number_of_notification_structer;
    for (uint8_t i = 0; i < n; i++)
    {
        if (!strcmp(ptr->notification_structer_ptr[i].topic, msg->topic))
        {
            cb = ptr->notification_structer_ptr[i].notification_function_ptr;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutex_lock);
    if (cb)
        cb((const char *)msg->payload, msg->topic);
}

// Internal: Connection callback
static void connection_callback(struct mosquitto *mosq, void *obj, int result)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    pthread_mutex_lock(&g_state_mutex);
    if (result == 0)
    {
        ptr->connection_state_flag = e_libmqttlink_connection_state_connection_true;
        pthread_mutex_unlock(&g_state_mutex);
        printf("%s(): Connection to Mosquitto server established.\n", __func__);
        return;
    }
    ptr->connection_state_flag = e_libmqttlink_connection_state_connection_false;
    pthread_mutex_unlock(&g_state_mutex);
    printf("%s(): Connection to Mosquitto server failed. Reason: [%s]\n", __func__, mosquitto_strerror(result));
}

// Internal: Subscribe to all registered topics
static int subscribe_all_topics(void)
{
    int subs_counter = 0;
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    const int MAX_ATTEMPTS = 10;
    int attempts = 0;
    while (attempts < MAX_ATTEMPTS)
    {
        subs_counter = 0;
        pthread_mutex_lock(&g_mutex_lock);
        for (int i = 0; i < ptr->number_of_notification_structer; ++i)
        {
            int *mid = NULL;
            int qos = ptr->notification_structer_ptr[i].qos;
            int result = mosquitto_subscribe(ptr->mosquitto_structer_ptr, mid, ptr->notification_structer_ptr[i].topic, qos);
            if (result != MOSQ_ERR_SUCCESS)
            {
                printf("%s(): Could not subscribe to topic [%s]. Reason: [%s]\n", __func__, ptr->notification_structer_ptr[i].topic, mosquitto_strerror(result));
            }
            else
            {
                printf("%s(): Subscribed to topic [%s].\n", __func__, ptr->notification_structer_ptr[i].topic);
                subs_counter++;
            }
        }
        pthread_mutex_unlock(&g_mutex_lock);
        if (subs_counter == ptr->number_of_notification_structer)
            return 0;
        attempts++;
        mqttlink_sleep_milisec(500 + attempts * 200);
    }
    printf("%s(): Failed to subscribe all topics after %d attempts.\n", __func__, MAX_ATTEMPTS);
    return -1;
}

// Internal: Unsubscribe from all topics
static int unsubscribe_all_topics(void)
{
    int unsubs_counter = 0;
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    const int MAX_ATTEMPTS = 10;
    int attempts = 0;
    while (attempts < MAX_ATTEMPTS)
    {
        unsubs_counter = 0;
        pthread_mutex_lock(&g_mutex_lock);
        for (int i = 0; i < ptr->number_of_notification_structer; ++i)
        {
            int *mid = NULL;
            int result = mosquitto_unsubscribe(ptr->mosquitto_structer_ptr, mid, ptr->notification_structer_ptr[i].topic);
            if (result != MOSQ_ERR_SUCCESS)
            {
                printf("%s(): Could not unsubscribe from topic [%s]. Reason: [%s]\n", __func__, ptr->notification_structer_ptr[i].topic, mosquitto_strerror(result));
            }
            else
            {
                printf("%s(): Unsubscribed from topic [%s].\n", __func__, ptr->notification_structer_ptr[i].topic);
                unsubs_counter++;
            }
        }
        pthread_mutex_unlock(&g_mutex_lock);
        if (unsubs_counter == ptr->number_of_notification_structer)
            return 0;
        attempts++;
        mqttlink_sleep_milisec(300 + attempts * 150);
    }
    printf("%s(): Failed to unsubscribe all topics after %d attempts.\n", __func__, MAX_ATTEMPTS);
    return -1;
}

// Internal: Thread function to manage connection and periodic restart
static void *connection_state_thread(void *login_info_ptr)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    mosquitto_lib_init();

    bool clean_session = false;
    char id[256];

    generate_client_id(id, sizeof(id));

    void *obj = NULL;
    ptr->mosquitto_structer_ptr = mosquitto_new(id, clean_session, obj);
    if (ptr->mosquitto_structer_ptr == NULL)
    {
        printf("%s(): Failed to start Mosquitto library. Memory error.\n", __func__);
        mosquitto_lib_cleanup();
        pthread_exit(NULL);
    }

    // Apply Will if configured
    if (ptr->will_topic && ptr->will_payload)
    {
        int rc = mosquitto_will_set(ptr->mosquitto_structer_ptr, ptr->will_topic, (int)strlen(ptr->will_payload), ptr->will_payload, ptr->will_qos, ptr->will_retain);
        if (rc != MOSQ_ERR_SUCCESS)
            printf("%s(): Failed to set will: %s\n", __func__, mosquitto_strerror(rc));
    }

    // Apply TLS if configured
    if (ptr->tls_cafile || ptr->tls_capath || ptr->tls_certfile || ptr->tls_keyfile)
    {
        int rc = mosquitto_tls_set(ptr->mosquitto_structer_ptr, ptr->tls_cafile, ptr->tls_capath, ptr->tls_certfile, ptr->tls_keyfile, NULL);
        if (rc != MOSQ_ERR_SUCCESS)
            printf("%s(): Failed to set TLS: %s\n", __func__, mosquitto_strerror(rc));
        if (ptr->tls_insecure)
            mosquitto_tls_insecure_set(ptr->mosquitto_structer_ptr, true);
        if (ptr->tls_version)
        {
            rc = mosquitto_tls_opts_set(ptr->mosquitto_structer_ptr, 1, ptr->tls_version, NULL);
            if (rc != MOSQ_ERR_SUCCESS)
                printf("%s(): Failed to set TLS opts: %s\n", __func__, mosquitto_strerror(rc));
        }
    }

    mosquitto_username_pw_set(ptr->mosquitto_structer_ptr, ptr->user_name, ptr->password);
    mosquitto_connect_callback_set(ptr->mosquitto_structer_ptr, connection_callback);
    mosquitto_message_callback_set(ptr->mosquitto_structer_ptr, message_received_callback);

    int keepalive = 60;
    int initial_connect_rc = mosquitto_connect(ptr->mosquitto_structer_ptr, ptr->server_ip_address, ptr->server_port, keepalive);
    if (initial_connect_rc != MOSQ_ERR_SUCCESS)
        printf("%s(): Initial connect failed: %s\n", __func__, mosquitto_strerror(initial_connect_rc));

    int max_packets = 1;
    int timeout = 1000;
    double last_restart_time = 0;
    volatile bool restart_flag = 0;

    // Reconnect backoff
    int backoff_ms = 500; // start
    const int max_backoff_ms = 30000;

    while (!g_stop_flag)
    {
        int result = mosquitto_loop(ptr->mosquitto_structer_ptr, timeout, max_packets);
        if (result != MOSQ_ERR_SUCCESS)
        {
            pthread_mutex_lock(&g_state_mutex);
            ptr->connection_state_flag = e_libmqttlink_connection_state_connection_false;
            pthread_mutex_unlock(&g_state_mutex);
            printf("%s(): mosquitto_loop(): Connection lost. Reason: [%s]\n", __func__, mosquitto_strerror(result));
            mqttlink_sleep_milisec(backoff_ms);
            mosquitto_reconnect(ptr->mosquitto_structer_ptr);
            // exponential backoff with cap
            backoff_ms *= 2;
            if (backoff_ms > max_backoff_ms)
                backoff_ms = max_backoff_ms;
            continue; // skip rest of loop until success
        }
        else
        {
            // reset backoff on success
            backoff_ms = 500;
        }

        if (result == MOSQ_ERR_SUCCESS && subsc_fonk_check_flag == 0)
        {
            subscribe_all_topics();
            subsc_fonk_check_flag = 1;
        }

        if (result == MOSQ_ERR_SUCCESS && restart_flag == 1)
        {
            subscribe_all_topics();
            restart_flag = 0;
        }

        double now = mqttlink_get_system_time();
        const int h24_sec = 86400;
        if ((now - last_restart_time) > h24_sec)
        {
            printf("%s(): mosquitto_loop(): Restarting broker connection!\n", __func__);
            last_restart_time = now;
            unsubscribe_all_topics();
            mqttlink_sleep_milisec(1000);
            mosquitto_reconnect(ptr->mosquitto_structer_ptr);
            mqttlink_sleep_milisec(1000);
            restart_flag = 1;
        }

        mqttlink_sleep_milisec(10);
    }
    pthread_exit(NULL);
}

/**
 * Establishes a connection to the MQTT broker and monitors the connection state.
 */
int libmqttlink_connect_and_monitor(const char *server_ip_address, int server_port, const char *user_name, const char *password)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    if (ptr->notification_structer_ptr != NULL)
    {
        printf("%s(): MQTT link already active.\n", __func__);
        return -1;
    }

    ptr->server_ip_address = server_ip_address;
    ptr->server_port = server_port;
    ptr->user_name = user_name;
    ptr->password = password;

    if (pthread_mutex_init(&g_mutex_lock, NULL) != 0)
    {
        fprintf(stderr, "%s(): Mutex init failed.\n", __func__);
        return -1; // standardized error code
    }
    if (pthread_mutex_init(&g_state_mutex, NULL) != 0)
    {
        fprintf(stderr, "%s(): State mutex init failed.\n", __func__);
        pthread_mutex_destroy(&g_mutex_lock);
        return -1;
    }
    g_stop_flag = false;

    int result = pthread_create(&ptr->link_control_thread_id, NULL, connection_state_thread, NULL);
    if (result)
    {
        printf("%s(): Thread could not be created. Reason: [%s]\n", __func__, strerror(result));
        return -1;
    }
    printf("%s(): Thread created. id: [%ld]\n", __func__, ptr->link_control_thread_id);
    return 0;
}

/**
 * Closes the MQTT broker connection and cleans up resources.
 */
void libmqttlink_shutdown(void)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    if (ptr->mosquitto_structer_ptr != NULL)
    {
        printf("%s(): Signaling connection control thread to stop.\n", __func__);
        g_stop_flag = true; // graceful stop
        pthread_join(ptr->link_control_thread_id, NULL);

        printf("%s(): Disconnecting from Mosquitto server.\n", __func__);
        if (ptr->connection_state_flag == e_libmqttlink_connection_state_connection_true)
        {
            mosquitto_disconnect(ptr->mosquitto_structer_ptr);
        }
        mosquitto_destroy(ptr->mosquitto_structer_ptr);
        mosquitto_lib_cleanup();

        if (ptr->notification_structer_ptr != NULL)
        {
            printf("%s(): Freeing subscriber memory.\n", __func__);
            free(ptr->notification_structer_ptr);
        }

        ptr->notification_structer_ptr = NULL;
        ptr->mosquitto_structer_ptr = NULL;
        ptr->connection_state_flag = e_libmqttlink_connection_state_connection_false;
    }
    pthread_mutex_destroy(&g_mutex_lock);
    pthread_mutex_destroy(&g_state_mutex);
}

/**
 * Publishes a message to a topic.
 */
int libmqttlink_publish_message(const char *topic, const char *message_contents, int qos, enum _enum_libmqttlink_message_storage_flag_state message_storage_flag_state)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    pthread_mutex_lock(&g_state_mutex);
    int disconnected = (ptr->connection_state_flag == e_libmqttlink_connection_state_connection_false);
    pthread_mutex_unlock(&g_state_mutex);
    if (disconnected)
    {
        printf("%s(): Message could not be sent. Connection state is false.\n", __func__);
        return -1;
    }

    int *mid = NULL;
    int message_length = strlen(message_contents);
    int result = mosquitto_publish(ptr->mosquitto_structer_ptr, mid, topic, message_length, message_contents, qos, message_storage_flag_state);
    if (result != 0)
    {
        printf("%s(): Message could not be sent. Result: [%d]\n", __func__, result);
        return -1;
    }

    mqttlink_sleep_milisec(1);
    return 0;
}

/**
 * Subscribes to a topic and sets a callback function for incoming messages.
 */
int libmqttlink_subscribe_topic(const char *topic, int qos, void (*notification_function_ptr)(const char *message_contents, const char *topic))
{
    if (notification_function_ptr == NULL || topic == NULL)
    {
        printf("%s(): NULL values are not allowed.\n", __func__);
        return -1;
    }
    if (qos < 0 || qos > 2)
        qos = 0; // sanitize

    size_t tlen = strlen(topic);
    if (tlen >= sizeof(((struct struct_notification_structer *)0)->topic))
    {
        printf("%s(): Topic too long.\n", __func__);
        return -1;
    }

    pthread_mutex_lock(&g_mutex_lock);

    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    uint8_t new_count = ptr->number_of_notification_structer + 1;
    struct struct_notification_structer *tmp = (struct struct_notification_structer *)realloc(ptr->notification_structer_ptr, new_count * sizeof(struct struct_notification_structer));
    if (!tmp)
    {
        printf("%s(): realloc() failed.\n", __func__);
        pthread_mutex_unlock(&g_mutex_lock);
        return -1;
    }
    ptr->notification_structer_ptr = tmp;
    ptr->number_of_notification_structer = new_count;

    int idx = new_count - 1;
    memcpy(ptr->notification_structer_ptr[idx].topic, topic, tlen + 1);
    ptr->notification_structer_ptr[idx].qos = qos;
    ptr->notification_structer_ptr[idx].notification_function_ptr = notification_function_ptr;

    subsc_fonk_check_flag = 0; // trigger re-subscribe in loop

    pthread_mutex_unlock(&g_mutex_lock);
    return 0;
}

/**
 * Unsubscribes from a topic.
 */
int libmqttlink_unsubscribe_topic(const char *topic)
{
    if (!topic)
        return -1;
    pthread_mutex_lock(&g_mutex_lock);
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    int found = -1;
    for (int i = 0; i < ptr->number_of_notification_structer; ++i)
    {
        if (strcmp(ptr->notification_structer_ptr[i].topic, topic) == 0)
        {
            found = i;
            break;
        }
    }
    if (found == -1)
    {
        pthread_mutex_unlock(&g_mutex_lock);
        return -1; // not found
    }
    // shift down
    for (int j = found; j < ptr->number_of_notification_structer - 1; ++j)
        ptr->notification_structer_ptr[j] = ptr->notification_structer_ptr[j + 1];
    ptr->number_of_notification_structer--;
    if (ptr->number_of_notification_structer == 0)
    {
        free(ptr->notification_structer_ptr);
        ptr->notification_structer_ptr = NULL;
    }
    else
    {
        struct struct_notification_structer *tmp = realloc(ptr->notification_structer_ptr, ptr->number_of_notification_structer * sizeof(*ptr->notification_structer_ptr));
        if (tmp)
            ptr->notification_structer_ptr = tmp; // ignore shrink failure
    }
    subsc_fonk_check_flag = 0; // trigger re-sync
    pthread_mutex_unlock(&g_mutex_lock);
    return 0;
}

/**
 * Sets the Last Will and Testament (LWT) for the MQTT connection.
 */
int libmqttlink_set_will(const char *topic, const char *payload, int qos, int retain)
{
    if (!topic || !payload)
        return -1;
    if (qos < 0 || qos > 2)
        qos = 0;
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    if (ptr->mosquitto_structer_ptr != NULL)
        return -1; // must be before connect
    ptr->will_topic = topic;
    ptr->will_payload = payload;
    ptr->will_qos = qos;
    ptr->will_retain = retain ? 1 : 0;
    return 0;
}

/**
 * Configures TLS settings for the MQTT connection.
 */
int libmqttlink_set_tls(const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char *tls_version, int insecure)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    if (ptr->mosquitto_structer_ptr != NULL)
        return -1; // set before connect
    ptr->tls_cafile = cafile;
    ptr->tls_capath = capath;
    ptr->tls_certfile = certfile;
    ptr->tls_keyfile = keyfile;
    ptr->tls_version = tls_version;
    ptr->tls_insecure = insecure ? 1 : 0;
    return 0;
}

/**
 * Returns the current connection state of the MQTT link.
 */
enum _enum_libmqttlink_connection_state libmqttlink_get_connection_state(void)
{
    enum _enum_libmqttlink_connection_state st;
    pthread_mutex_lock(&g_state_mutex);
    st = g_libmqttlink_struct.connection_state_flag;
    pthread_mutex_unlock(&g_state_mutex);
    return st;
}