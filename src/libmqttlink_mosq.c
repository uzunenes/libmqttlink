#include "../include/libmqttlink_mosq.h"
#include "../include/libmqttlink_utility_functions.h"

#include <pthread.h>
#include <mosquitto.h>
#include <string.h>
#ifdef OS_Linux
    #include <stdio.h>
#endif
#ifdef OS_Windows
    #include <stdio.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>


struct struct_notification_structer
{
    void (*notification_function_ptr)(const char *message_content, const char *subject);
    char subject[1024];
    pthread_t thread_id;
};

struct struct_mqttlink_mosq_struct
{
    struct mosquitto *mosquitto_structer_ptr;
    struct struct_notification_structer *notification_structer_ptr;
    uint8_t number_of_notification_structer;
    const char *server_ip_address;
    uint16_t server_port;
    const char *user_name;
    const char *password;
    pthread_t link_control_thread_id;
    enum _enum_mqttlink_mosq_connection_state connection_state_flag;
};

// -- End of build declaration demos --


// -- start of 'static' function notification definitions --

/*
* Instruction: It is the function where the messages of the subscribed subjects come.
*
* Parameters:
    @notification_structer_ptr: Mosquitto structer pointer
    @obj: Not used.
    @mosquitto_message_structer_ptr: Mosquitto message structer pointer.
*
* Retunt Value: none
*/          
static void  mosq_message_received_notification_function(struct mosquitto *notification_structer_ptr, void *obj, const struct mosquitto_message *mosquitto_message_structer_ptr);

/*
* Instruction: The result of the request sent to establish the connection comes to this function.
*
* Parameters:
    @notification_structer_ptr: Not used.
    @obj: Not used.
    @result: if the result is Zero(0). The connection has been established.
*
* Return Value: none
*/
static void mosq_connection_info_arrived_notification_function(struct mosquitto *notification_structer_ptr, void *obj, int result);

/*
* Instruction: Creates a flow in a thread to follow the Mosquitto link.
*
* Parametreler:
    @login_info_ptr: none, not used.
*
* Retunt Value: none.
*/
static void* mosq_connection_state_flow_function(void *login_info_ptr);

// -- End of 'static' function declaration definitions --

// --- Beginning of global variables ---

static struct struct_mqttlink_mosq_struct g_mqttlink_mosq_struct = {
    .mosquitto_structer_ptr = NULL,
    .notification_structer_ptr = NULL,
    .number_of_notification_structer = 0,
    .server_ip_address = NULL,
    .server_port = 0,
    .user_name = NULL,
    .password = NULL,
    .link_control_thread_id = 0,
    .connection_state_flag = e_mqttlink_mosq_connection_state_connection_false,
};

static pthread_mutex_t g_mutex_lock; 
static volatile bool subsc_fonk_check_flag = 0;

// --- End of global variables ---

static void
mosq_message_received_notification_function(struct mosquitto *notification_structer_ptr, void *obj, const struct mosquitto_message *mosquitto_message_structer_ptr)
{
    uint8_t subscribe_struct_counter;
    uint8_t number_of_notification_structer;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;
    number_of_notification_structer = g_mqttlink_mosq_struct_ptr->number_of_notification_structer;
    for( subscribe_struct_counter = 0; subscribe_struct_counter < number_of_notification_structer; subscribe_struct_counter++ )
    {
        if( !strcmp( g_mqttlink_mosq_struct_ptr->notification_structer_ptr[subscribe_struct_counter].subject, mosquitto_message_structer_ptr->topic ) )
        {
            g_mqttlink_mosq_struct_ptr->notification_structer_ptr[subscribe_struct_counter].notification_function_ptr((char *)mosquitto_message_structer_ptr->payload, mosquitto_message_structer_ptr->topic);
            return;
        }
    }
}

static void
mosq_connection_info_arrived_notification_function(struct mosquitto *notification_structer_ptr, void *obj, int result)
{
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    if( result == 0 )
    {
        g_mqttlink_mosq_struct_ptr->connection_state_flag = e_mqttlink_mosq_connection_state_connection_true;

#ifdef OS_Linux
        printf("%s(): Connection to Mosquitto server established.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): Connection to Mosquitto server established.", __func__));
#endif

        return;
    }

#ifdef OS_Linux
    printf("%s(): The connection to the Mosquitto server could not be established. Explanation: [%s].", __func__, mosquitto_strerror(result));
#endif
#ifdef OS_Windows
    printf("%s(): The connection to the Mosquitto server could not be established. Explanation: [%s].", __func__, mosquitto_strerror(result));
#endif

}

static int
subscribe_all_topic(void)
{
    int subs_counter = 0;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    while(1)
    {
        for( int i=0; i < g_mqttlink_mosq_struct_ptr->number_of_notification_structer; ++i )
        {
            int* mid = NULL; // a pointer to an int.  If not NULL, the function will set this to the message id of this particular message. This can be then used with the subscribe callback to determine when the message has been sent.
            int qos = 2; // the requested Quality of Service for this subscription.
            int result = mosquitto_subscribe(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, mid, g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject, qos);
            if ( result != MOSQ_ERR_SUCCESS )
            {
#ifdef OS_Linux
                printf("%s(): could not subscribe to the topic. Explanation: [%s] - topic: [%s].", __func__, mosquitto_strerror(result), g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
#ifdef OS_Windows
                printf("%s(): could not subscribe to the topic. Explanation: [%s] - topic: [%s].", __func__, mosquitto_strerror(result), g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
            }
            else
            {
#ifdef OS_Linux
            printf("%s(): Subscribe success to the topic. topic: [%s].", __func__, g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
#ifdef OS_Windows
            printf("%s(): Subscribe success to the topic. topic: [%s].", __func__, g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
                subs_counter++;
            }
        }

        if(subs_counter == g_mqttlink_mosq_struct_ptr->number_of_notification_structer)
        {
            return 0;
        }

        subs_counter = 0;
        mqttlink_sleep_milisec(1000);
    }
}

static int
unsubscribe_all_topic(void)
{
    int unsubs_counter = 0;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    while(1)
    {
        for( int i=0; i < g_mqttlink_mosq_struct_ptr->number_of_notification_structer; ++i )
        {
            int* mid = NULL; // a pointer to an int.  If not NULL, the function will set this to the message id of this particular message. This can be then used with the subscribe callback to determine when the message has been sent.
            int result = mosquitto_unsubscribe(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, mid, g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);            if ( result != MOSQ_ERR_SUCCESS )
            {
#ifdef OS_Linux
                printf("%s(): could not unsubscribe to the topic. Explanation: [%s] - topic: [%s].", __func__, mosquitto_strerror(result), g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
#ifdef OS_Windows
                printf("%s(): could not unsubscribe to the topic. Explanation: [%s] - topic: [%s].", __func__, mosquitto_strerror(result), g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
            }
            else
            {
#ifdef OS_Linux
            printf("%s(): Unsubscribe success to the topic. topic: [%s].", __func__, g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
#ifdef OS_Windows
            printf("%s(): Unsubscribe success to the topic. topic: [%s].", __func__, g_mqttlink_mosq_struct_ptr->notification_structer_ptr[i].subject);
#endif
                unsubs_counter++;
            }
        }

        if(unsubs_counter == g_mqttlink_mosq_struct_ptr->number_of_notification_structer)
        {
            return 0;
        }

        unsubs_counter = 0;
        mqttlink_sleep_milisec(1000);
    }
}


static void*
mosq_connection_state_flow_function(void *login_info_ptr)
{
    bool clean_session;
    char id[512];
    void *obj;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;
    int keepalive;
    volatile int result;
    int max_packets;
    int timeout;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    mosquitto_lib_init();

    //------------------------------------------------------------------------------------------------------------------------------

    /*
    * set to true to instruct the broker to clean all messages and subscriptions on disconnect, false to instruct it to keep them.
    * See the man page mqtt(7) for more details.
    * Note that a client will never discard its own outgoing messages on disconnect.
    * Calling mosquitto_connect or mosquitto_reconnect will cause the messages to be resent.
    * Use mosquitto_reinitialise to reset a client to its original state.  Must be set to true if the id parameter is NULL.
    */
    clean_session = false;

    /*
    * String to use as the client id. If NULL, a random client id will be generated.
    * If id is NULL, clean_session must be true.
    */
    char add[128];
    char date_time[128];
    mqttlink_get_current_system_time_and_date(date_time);
    if( get_primary_IP(add) == 0 )
    {
        sprintf(id, "%s-%s", add, date_time);
    }
    else
    {
        sprintf(id, "%s", date_time); 
    }

    /*
    * A user pointer that will be passed as an argument to any callbacks that are specified.
    */
    obj = NULL;
    g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr = mosquitto_new(id, clean_session, obj);
    if( g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr == NULL )
    {

#ifdef OS_Linux
        printf("%s(): Failed to start Mosquitto library. memory error.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): Failed to start Mosquitto library. Memory error.", __func__);
#endif

        pthread_exit(NULL);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    mosquitto_username_pw_set(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, g_mqttlink_mosq_struct_ptr->user_name, g_mqttlink_mosq_struct_ptr->password);
    //------------------------------------------------------------------------------------------------------------------------------

    mosquitto_connect_callback_set(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, mosq_connection_info_arrived_notification_function);
    mosquitto_message_callback_set(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, mosq_message_received_notification_function);

    keepalive = 60;
    mosquitto_connect(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, g_mqttlink_mosq_struct_ptr->server_ip_address, g_mqttlink_mosq_struct_ptr->server_port, keepalive);

    max_packets = 1; // this parameter is currently unused and should be set to 1 for future compatibility.
    timeout = 1000; // Maximum number of milliseconds to wait for network activity in the select() call before timing out. Set to 0 for instant return. Set negative to use the default of 1000ms.
    
    double last_restart_time = 0;
    volatile bool restart_flag = 0;

    while( 1 )
    {
        result = mosquitto_loop(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, timeout, max_packets);
        if( result != MOSQ_ERR_SUCCESS )
        {
            g_mqttlink_mosq_struct_ptr->connection_state_flag = e_mqttlink_mosq_connection_state_connection_false;

#ifdef OS_Linux
        printf("%s(): mosquitto_loop(): Connection disappeare. Explanation: [%s].", __func__, mosquitto_strerror(result));
#endif
#ifdef OS_Windows
        printf("%s(): mosquitto_loop():Connection disappeare. Explanation: [%s].", __func__, mosquitto_strerror(result));
#endif
            mqttlink_sleep_milisec(1000);
            mosquitto_reconnect(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr);
        }

        if ( result == MOSQ_ERR_SUCCESS  && subsc_fonk_check_flag == 0 )
        {
            subscribe_all_topic();
            subsc_fonk_check_flag = 1;
        }

        if ( result == MOSQ_ERR_SUCCESS && restart_flag == 1 )
        {
            subscribe_all_topic();
            restart_flag = 0;
        }

        double now = mqttlink_get_system_time();
        const int h24_sec = 86400;
        if ( (now - last_restart_time) > h24_sec )
        {
            printf("%s(): mosquitto_loop(): Restarting broker connection!", __func__);
            last_restart_time = now;
            unsubscribe_all_topic();
            mqttlink_sleep_milisec(1000);
            mosquitto_reconnect(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr);
            mqttlink_sleep_milisec(1000);
            restart_flag = 1;
        }

        mqttlink_sleep_milisec(10);
    }

    pthread_exit(NULL);
}

int
mqttlink_establishes_mosq_connection_and_follows(const char *server_ip_address, int server_port, const char *user_name, const char *password)
{
    int result;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    if( g_mqttlink_mosq_struct_ptr->notification_structer_ptr != NULL )
    {

#ifdef OS_Linux
        printf("%s(): mosq already active.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): mosq already active.", __func__);
#endif

        return -1;
    }

    g_mqttlink_mosq_struct_ptr->server_ip_address = server_ip_address;
    g_mqttlink_mosq_struct_ptr->server_port = server_port;
    g_mqttlink_mosq_struct_ptr->user_name = user_name;
    g_mqttlink_mosq_struct_ptr->password = password;

    if (pthread_mutex_init(&g_mutex_lock, NULL) != 0)
    { 
#ifdef OS_Linux
        printf("%s(): Mutex init has failed.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): Mutex init has failed.", __func__);
#endif
        return 1; 
    } 

    result = pthread_create(&g_mqttlink_mosq_struct_ptr->link_control_thread_id, NULL, mosq_connection_state_flow_function, NULL);
    if( result )
    {

#ifdef OS_Linux
        printf("%s(): Thread could not be created. Explanation: [%s].", __func__, strerror(result));
#endif
#ifdef OS_Windows
        printf("%s(): Thread could not be created. Explanation: [%s].", __func__, strerror(result));
#endif

        return -1;
    }

#ifdef OS_Linux
        printf("%s(): Thread is created. id: [%ld].", __func__, g_mqttlink_mosq_struct_ptr->link_control_thread_id);
#endif
#ifdef OS_Windows
        printf("%s(): Thread is created. id: [%ld].", __func__, g_mqttlink_mosq_struct_ptr->link_control_thread_id);
#endif

    return 0;
}

int
mqttlink_subscribe_mosq_subject(const char *subject, void (*notification_function_ptr)(const char *message_content, const char *subject))
{
    int current_subscriber_array_index_number;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    if( notification_function_ptr == NULL || subject == NULL )
    {

#ifdef OS_Linux
        printf("%s(): You cannot enter NULL values.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): You cannot enter NULL values.", __func__);
#endif

        return -1;
    }

    pthread_mutex_lock(&g_mutex_lock); 

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    ++g_mqttlink_mosq_struct_ptr->number_of_notification_structer;
    g_mqttlink_mosq_struct_ptr->notification_structer_ptr = (struct struct_notification_structer *)realloc(g_mqttlink_mosq_struct_ptr->notification_structer_ptr, g_mqttlink_mosq_struct_ptr->number_of_notification_structer* sizeof(struct struct_notification_structer));
    if( g_mqttlink_mosq_struct_ptr->notification_structer_ptr == NULL )
    {

#ifdef OS_Linux
        printf("%s(): realloc() error. Number of the subscribe struct: [%d].", __func__, g_mqttlink_mosq_struct_ptr->number_of_notification_structer);
#endif
#ifdef OS_Windows
        printf("%s(): realloc() error. Number of the subscribe struct: [%d].", __func__, g_mqttlink_mosq_struct_ptr->number_of_notification_structer);
#endif

    }

    current_subscriber_array_index_number = g_mqttlink_mosq_struct_ptr->number_of_notification_structer - 1;
    strcpy(g_mqttlink_mosq_struct_ptr->notification_structer_ptr[current_subscriber_array_index_number].subject, subject);
    g_mqttlink_mosq_struct_ptr->notification_structer_ptr[current_subscriber_array_index_number].notification_function_ptr = notification_function_ptr;


    subsc_fonk_check_flag = 0; // subscribe flag open

    pthread_mutex_unlock(&g_mutex_lock); 

    return 0;
}

void
mqttlink_finished_mosq(void)
{
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    if( g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr != NULL )
    {

#ifdef OS_Linux
        printf("%s(): pthread_cancel(), The connection control thread is being terminated.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): pthread_cancel(), The connection control thread is being terminated.", __func__);
#endif

        pthread_cancel(g_mqttlink_mosq_struct_ptr->link_control_thread_id);

#ifdef OS_Linux
        printf("%s(): Terminating connection to Mosquitto server.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): Terminating connection to Mosquitto server.", __func__);
#endif

        if( g_mqttlink_mosq_struct_ptr->connection_state_flag == e_mqttlink_mosq_connection_state_connection_true )
        {
            mosquitto_disconnect(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr);
        }
        mosquitto_destroy(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr);
        mosquitto_lib_cleanup();

        if( g_mqttlink_mosq_struct_ptr->notification_structer_ptr != NULL )
        {

#ifdef OS_Linux
            printf("%s(): Freeing memory areas reserved for Mosquitto subscriber structure.", __func__);
#endif
#ifdef OS_Windows
            printf("%s(): Freeing memory areas reserved for Mosquitto subscriber structure.", __func__);
#endif

            free(g_mqttlink_mosq_struct_ptr->notification_structer_ptr);
        }

        // baslangic durumundaki degerlere degiskenler ayarlanir.
        g_mqttlink_mosq_struct_ptr->notification_structer_ptr = NULL;
        g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr = NULL;
        g_mqttlink_mosq_struct_ptr->connection_state_flag = e_mqttlink_mosq_connection_state_connection_false;
    }

    pthread_mutex_destroy(&g_mutex_lock);
}

int
mqttlink_send_mosq_message(const char *subject, const char *message_content, enum _enum_mqttlink_mosq_message_storage_flag_state message_storage_flag)
{
    int qos;
    int *mid;
    int result;
    int message_length;
    struct struct_mqttlink_mosq_struct *g_mqttlink_mosq_struct_ptr;

    g_mqttlink_mosq_struct_ptr = &g_mqttlink_mosq_struct;

    if( g_mqttlink_mosq_struct_ptr->connection_state_flag == e_mqttlink_mosq_connection_state_connection_false )
    {
#ifdef OS_Linux
        printf("%s(): Message could not be sent. Connection state false.", __func__);
#endif
#ifdef OS_Windows
        printf("%s(): Message could not be sent. Connection state false.", __func__);
#endif  
        return -1;
    }

    qos = 2; // integer value 0, 1 or 2 indicating the Quality of Service to be used for the message.
    mid = NULL; // pointer to an int.  If not NULL, the function will set this to the message id of this particular message.  This can be then used with the publish callback to determine when the message has been sent.
                // Note that although the MQTT protocol does not use message ids for messages with QoS=0, libmosquitto assigns them message ids so they can be tracked with this parameter.
    message_length = strlen(message_content);
    result = mosquitto_publish(g_mqttlink_mosq_struct_ptr->mosquitto_structer_ptr, mid, subject, message_length, message_content, qos, message_storage_flag);
    if( result != 0 )
    {
#ifdef OS_Linux
        printf("%s(): Message could not be sent. Result: [%d].", __func__, result);
#endif
#ifdef OS_Windows
        printf("%s(): Message could not be sent. Result: [%d].", __func__, result);
#endif
        return -1;
    }

    // We need a short delay here, to prevent the Mosquito library being torn down by the operating system before all the network operations are finished.
    mqttlink_sleep_milisec(1);

    return 0;
}

enum
_enum_mqttlink_mosq_connection_state mqttlink_get_mosq_connection_state(void)
{
    return g_mqttlink_mosq_struct.connection_state_flag == e_mqttlink_mosq_connection_state_connection_false ? e_mqttlink_mosq_connection_state_connection_false : e_mqttlink_mosq_connection_state_connection_true;
}                                                           

