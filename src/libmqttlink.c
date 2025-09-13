#include "../include/libmqttlink.h"
#include "../include/libmqttlink_utility_functions.h"

#include <pthread.h>
#include <mosquitto.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

// Structure for notification callback and topic
struct struct_notification_structer
{
    void (*notification_function_ptr)(const char *message_contents, const char *subject);
    char subject[1024];
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
};

static pthread_mutex_t g_mutex_lock; 
static volatile bool subsc_fonk_check_flag = 0;

// Internal: Dispatch received messages to the correct callback
static void message_received_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    uint8_t n = ptr->number_of_notification_structer;
    for (uint8_t i = 0; i < n; i++)
    {
        if (!strcmp(ptr->notification_structer_ptr[i].subject, msg->topic))
        {
            ptr->notification_structer_ptr[i].notification_function_ptr(
                (char *)msg->payload,
                msg->topic
            );
            return;
        }
    }
}

// Internal: Connection callback
static void connection_callback(struct mosquitto *mosq, void *obj, int result)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    if (result == 0)
    {
        ptr->connection_state_flag = e_libmqttlink_connection_state_connection_true;
        printf("%s(): Connection to Mosquitto server established.\n", __func__);
        return;
    }
    printf("%s(): Connection to Mosquitto server failed. Reason: [%s]\n", __func__, mosquitto_strerror(result));
}

// Internal: Subscribe to all registered topics
static int subscribe_all_topics(void)
{
    int subs_counter = 0;
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    while (1)
    {
        for (int i = 0; i < ptr->number_of_notification_structer; ++i)
        {
            int *mid = NULL;
            int qos = 2;
            int result = mosquitto_subscribe(
                ptr->mosquitto_structer_ptr,
                mid,
                ptr->notification_structer_ptr[i].subject,
                qos
            );
            if (result != MOSQ_ERR_SUCCESS)
            {
                printf("%s(): Could not subscribe to topic [%s]. Reason: [%s]\n", __func__,
                       ptr->notification_structer_ptr[i].subject,
                       mosquitto_strerror(result));
            }
            else
            {
                printf("%s(): Subscribed to topic [%s].\n", __func__, ptr->notification_structer_ptr[i].subject);
                subs_counter++;
            }
        }
        if (subs_counter == ptr->number_of_notification_structer)
            return 0;
        subs_counter = 0;
        libmqttlink_sleep_milisec(1000);
    }
}

// Internal: Unsubscribe from all topics
static int unsubscribe_all_topics(void)
{
    int unsubs_counter = 0;
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    while (1)
    {
        for (int i = 0; i < ptr->number_of_notification_structer; ++i)
        {
            int *mid = NULL;
            int result = mosquitto_unsubscribe(
                ptr->mosquitto_structer_ptr,
                mid,
                ptr->notification_structer_ptr[i].subject
            );
            if (result != MOSQ_ERR_SUCCESS)
            {
                printf("%s(): Could not unsubscribe from topic [%s]. Reason: [%s]\n", __func__,
                       ptr->notification_structer_ptr[i].subject,
                       mosquitto_strerror(result));
            }
            else
            {
                printf("%s(): Unsubscribed from topic [%s].\n", __func__, ptr->notification_structer_ptr[i].subject);
                unsubs_counter++;
            }
        }
        if (unsubs_counter == ptr->number_of_notification_structer)
            return 0;
        unsubs_counter = 0;
        libmqttlink_sleep_milisec(1000);
    }
}

// Internal: Thread function to manage connection and periodic restart
static void* connection_state_thread(void *login_info_ptr)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    mosquitto_lib_init();

    bool clean_session = false;
    char id[512];
    char add[128];
    char date_time[128];
    libmqttlink_get_current_system_time_and_date(date_time);
    if (libmqttlink_get_primary_IP(add) == 0)
        sprintf(id, "%s-%s", add, date_time);
    else
        sprintf(id, "%s", date_time);

    void *obj = NULL;
    ptr->mosquitto_structer_ptr = mosquitto_new(id, clean_session, obj);
    if (ptr->mosquitto_structer_ptr == NULL)
    {
        printf("%s(): Failed to start Mosquitto library. Memory error.\n", __func__);
        pthread_exit(NULL);
    }

    mosquitto_username_pw_set(ptr->mosquitto_structer_ptr, ptr->user_name, ptr->password);
    mosquitto_connect_callback_set(ptr->mosquitto_structer_ptr, connection_callback);
    mosquitto_message_callback_set(ptr->mosquitto_structer_ptr, message_received_callback);

    int keepalive = 60;
    mosquitto_connect(ptr->mosquitto_structer_ptr, ptr->server_ip_address, ptr->server_port, keepalive);

    int max_packets = 1;
    int timeout = 1000;
    double last_restart_time = 0;
    volatile bool restart_flag = 0;

    while (1)
    {
        int result = mosquitto_loop(ptr->mosquitto_structer_ptr, timeout, max_packets);
        if (result != MOSQ_ERR_SUCCESS)
        {
            ptr->connection_state_flag = e_libmqttlink_connection_state_connection_false;
            printf("%s(): mosquitto_loop(): Connection lost. Reason: [%s]\n", __func__, mosquitto_strerror(result));
            libmqttlink_sleep_milisec(1000);
            mosquitto_reconnect(ptr->mosquitto_structer_ptr);
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

        double now = libmqttlink_get_system_time();
        const int h24_sec = 86400;
        if ((now - last_restart_time) > h24_sec)
        {
            printf("%s(): mosquitto_loop(): Restarting broker connection!\n", __func__);
            last_restart_time = now;
            unsubscribe_all_topics();
            libmqttlink_sleep_milisec(1000);
            mosquitto_reconnect(ptr->mosquitto_structer_ptr);
            libmqttlink_sleep_milisec(1000);
            restart_flag = 1;
        }

        libmqttlink_sleep_milisec(10);
    }
    pthread_exit(NULL);
}

/**
 * Establishes a connection to the MQTT broker and monitors the connection state.
 */
int libmqttlink_establishes_connection_and_follows(const char *server_ip_address, int server_port, const char *user_name, const char *password)
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
        printf("%s(): Mutex init failed.\n", __func__);
        return 1;
    }

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
        printf("%s(): Terminating connection control thread.\n", __func__);
        pthread_cancel(ptr->link_control_thread_id);

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
}

/**
 * Publishes a message to a topic.
 */
int libmqttlink_send_message(const char *subject, const char *message_contents, enum _enum_libmqttlink_message_storage_flag_state message_storage_flag_state)
{
    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    if (ptr->connection_state_flag == e_libmqttlink_connection_state_connection_false)
    {
        printf("%s(): Message could not be sent. Connection state is false.\n", __func__);
        return -1;
    }

    int qos = 2;
    int *mid = NULL;
    int message_length = strlen(message_contents);
    int result = mosquitto_publish(
        ptr->mosquitto_structer_ptr,
        mid,
        subject,
        message_length,
        message_contents,
        qos,
        message_storage_flag_state
    );
    if (result != 0)
    {
        printf("%s(): Message could not be sent. Result: [%d]\n", __func__, result);
        return -1;
    }

    libmqttlink_sleep_milisec(1);
    return 0;
}

/**
 * Subscribes to a topic and sets a callback function for incoming messages.
 */
int libmqttlink_subscribe_subject(const char *subject, void (*notification_function_ptr)(const char *message_contents, const char *subject))
{
    if (notification_function_ptr == NULL || subject == NULL)
    {
        printf("%s(): NULL values are not allowed.\n", __func__);
        return -1;
    }

    pthread_mutex_lock(&g_mutex_lock);

    struct struct_libmqttlink_struct *ptr = &g_libmqttlink_struct;
    ++ptr->number_of_notification_structer;
    ptr->notification_structer_ptr = (struct struct_notification_structer *)realloc(
        ptr->notification_structer_ptr,
        ptr->number_of_notification_structer * sizeof(struct struct_notification_structer)
    );
    if (ptr->notification_structer_ptr == NULL)
    {
        printf("%s(): realloc() error. Number of subscribers: [%d]\n", __func__, ptr->number_of_notification_structer);
    }

    int idx = ptr->number_of_notification_structer - 1;
    strcpy(ptr->notification_structer_ptr[idx].subject, subject);
    ptr->notification_structer_ptr[idx].notification_function_ptr = notification_function_ptr;

    subsc_fonk_check_flag = 0; // subscribe flag open

    pthread_mutex_unlock(&g_mutex_lock);

    return 0;
}

/**
 * Returns the current connection state of the MQTT link.
 */
enum _enum_libmqttlink_connection_state libmqttlink_get_connection_state(void)
{
    return g_libmqttlink_struct.connection_state_flag;
}