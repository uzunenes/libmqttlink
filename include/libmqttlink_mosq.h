#ifndef mqttlink_MOSQ_H
#define mqttlink_MOSQ_H

#ifdef __cplusplus
	extern "C"
	{
#endif

// -- Yapı bildirim tanıtımları başlangıcı --

enum _enum_mqttlink_mosq_connection_state
{
	e_mqttlink_mosq_connection_state_connection_false,
	e_mqttlink_mosq_connection_state_connection_true
};

enum _enum_mqttlink_mosq_message_storage_flag_state
{
	e_mqttlink_mosq_message_storage_flag_state_message_dont_keep,
	e_mqttlink_mosq_message_storage_flag_state_message_keep
};

/**
*   Establishes the Mosquitto connection and follows to connection state. 
*	@param server_ip_adress: IP address of Mosquitto server.
*	@param server_port: Port information of Mosquitto server.
*	@param user_name: Mosquitto user name.
*	@param password: Mosquitto password.
*   @return Zero(0) successful status, minus one(-1)erroneous state.
**/
int mqttlink_establishes_mosq_connection_and_follows(const char *server_ip_address, int server_port, const char *user_name, const char *password);

/**
* 	 Closes Mosquitto connection.
*	 @param: None
* 	 @return None
*/
void mqttlink_finished_mosq(void);

/**
*   closes Mosquitto connection.
*	@param subject: the subject of the message to be sent.
*	@param message_contents: message contents.
*	@param message_keeping_flag_state: The enum flag that controls the retention of the last message by the broker..
* 	@return Zero(0) successfull state, negative values erroneous state.
**/
int mqttlink_send_mosq_message(const char *subject, const char *message_contents, enum _enum_mqttlink_mosq_message_storage_flag_state message_storage_flag_state);


/**
*   closes Mosquitto connection. 
*	@param subject: messagein subjectsu.
*	@param Nonetification_function_ptr: subject message gelecegi fonksiyon isaretcisi.
* 	@return  Zero(0) successfull state, negative values erroneous state.
**/
int mqttlink_subscribe_mosq_subject(const char *subject, void (*Nonetification_function_ptr)(const char *message_contents, const char *subject));


/**
*  gives Mosquitto connection status information.
*  @param None
*  @return connection state value type enum _enum_mqttlink_mosq_connection_state.
**/
enum _enum_mqttlink_mosq_connection_state mqttlink_get_mosq_connection_state(void);

#ifdef __cplusplus
	}
#endif

#endif // LIBISY_MOSQ_H
