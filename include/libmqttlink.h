#ifndef LIBMQTTLINK_H
#define LIBMQTTLINK_H

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
int libmqttlink_connect_and_monitor(const char *server_ip_address, int server_port, const char *user_name, const char *password);

/**
 * Closes the MQTT broker connection and cleans up resources.
 */
void libmqttlink_shutdown(void);

/**
 * Publishes a message to a topic.
 * @param topic Topic to publish the message to.
 * @param message_contents Message content.
 * @param qos Quality of Service level.
 * @param message_storage_flag_state Flag to control message retention by the broker.
 * @return 0 on success, negative value on error.
 */
int libmqttlink_publish_message(const char *topic, const char *message_contents, int qos, enum _enum_libmqttlink_message_storage_flag_state message_storage_flag_state);

/**
 * Subscribes to a topic and sets a callback function for incoming messages.
 * @param topic Topic to subscribe to.
 * @param qos Quality of Service level.
 * @param notification_function_ptr Callback function for received messages.
 * @return 0 on success, negative value on error.
 */
int libmqttlink_subscribe_topic(const char *topic, int qos, void (*notification_function_ptr)(const char *message_contents, const char *topic));

/**
 * Unsubscribes from a topic.
 * @param topic Topic to unsubscribe from.
 * @return 0 on success, negative value on error.
 */
int libmqttlink_unsubscribe_topic(const char *topic);

/**
 * Sets the Last Will message.
 * @param topic Topic for the Last Will message.
 * @param payload Payload of the Last Will message.
 * @param qos Quality of Service level.
 * @param retain Retain flag for the Last Will message.
 * @return 0 on success, negative value on error.
 */
int libmqttlink_set_will(const char *topic, const char *payload, int qos, int retain);

/**
 * Sets TLS parameters for the connection.
 * @param cafile Path to the CA file.
 * @param capath Path to the CA directory.
 * @param certfile Path to the client certificate file.
 * @param keyfile Path to the client key file.
 * @param tls_version TLS version to use.
 * @param insecure Insecure flag (1 to allow insecure connections, 0 otherwise).
 * @return 0 on success, negative value on error.
 */
int libmqttlink_set_tls(const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char *tls_version, int insecure);

/**
 * Returns the current connection state of the MQTT link.
 * @return Connection state as enum _enum_libmqttlink_connection_state.
 */
enum _enum_libmqttlink_connection_state libmqttlink_get_connection_state(void);

#ifdef __cplusplus
}
#endif

#endif // LIBMQTTLINK_H