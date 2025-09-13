#ifndef libmqttlink_H
#define libmqttlink_H

#ifdef __cplusplus
extern "C" {
#endif

// -- Structure and enum declarations --

/**
 * Connection state for MQTT link.
 */
enum _enum_libmqttlink_connection_state
{
    e_libmqttlink_connection_state_connection_false,
    e_libmqttlink_connection_state_connection_true
};

/**
 * Message storage flag for MQTT broker.
 */
enum _enum_libmqttlink_message_storage_flag_state
{
    e_libmqttlink_message_storage_flag_state_message_dont_keep,
    e_libmqttlink_message_storage_flag_state_message_keep
};

/**
 * Establishes a connection to the MQTT broker and monitors the connection state.
 * @param server_ip_address IP address of the MQTT broker.
 * @param server_port Port number of the MQTT broker.
 * @param user_name Username for authentication.
 * @param password Password for authentication.
 * @return 0 on success, -1 on error.
 */
int libmqttlink_establishes_mosq_connection_and_follows(const char *server_ip_address, int server_port, const char *user_name, const char *password);

/**
 * Closes the MQTT broker connection and cleans up resources.
 */
void libmqttlink_finished_mosq(void);

/**
 * Publishes a message to a topic.
 * @param subject Topic to publish the message to.
 * @param message_contents Message content.
 * @param message_storage_flag_state Flag to control message retention by the broker.
 * @return 0 on success, negative value on error.
 */
int libmqttlink_send_mosq_message(const char *subject, const char *message_contents, enum _enum_libmqttlink_message_storage_flag_state message_storage_flag_state);

/**
 * Subscribes to a topic and sets a callback function for incoming messages.
 * @param subject Topic to subscribe to.
 * @param notification_function_ptr Callback function for received messages.
 * @return 0 on success, negative value on error.
 */
int libmqttlink_subscribe_mosq_subject(const char *subject, void (*notification_function_ptr)(const char *message_contents, const char *subject));

/**
 * Returns the current connection state of the MQTT link.
 * @return Connection state as enum _enum_libmqttlink_connection_state.
 */
enum _enum_libmqttlink_connection_state libmqttlink_get_mosq_connection_state(void);

#ifdef __cplusplus
}
#endif

#endif // libmqttlink_H