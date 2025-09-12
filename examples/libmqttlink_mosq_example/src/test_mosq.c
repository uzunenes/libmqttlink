#include <signal.h> // signal()
#include <stdio.h>
#include <libmqttlink/libmqttlink_mosq.h>
#include <libmqttlink/libmqttlink_utility_functions.h>

// -- 'static' start of function declaration definitions --

/*
* Instruction: Checks the output signal of the program
*
* Params:
    @sig: signal information
*
* Donus Degeri: none
*/
static void cik(int sig);

/*
* Instruction: Notification function to call the incoming message
*
* Params:
    @message: Incoming message information.
	@subject: Subject of incoming message.
*
* Donus Degeri: none
*/
static void message_arrived_notification_function(const char *message, const char *subject);

// -- End of 'static' function declaration instructions --


// -- Beginning of global variables --

static int g_cikis_sinyali = 0;

// -- End of global variables --


int
main(void)
{
    const char *server_ip_address = 
    int server_port = 1883;
    const char *password = 
    const char *user_name = 

	// Subject examples
    const char *sbj1 = "mqttlink_test";
    const char *sbj2 = "mqttlink_test2";

    signal(SIGINT, cik);
    signal(SIGTERM, cik);


    mqttlink_establishes_mosq_connection_and_follows(server_ip_address, server_port, user_name, password);

    mqttlink_subscribe_mosq_subject(sbj1, message_arrived_notification_function);
    mqttlink_subscribe_mosq_subject(sbj2, message_arrived_notification_function);


    int i = 0;    
    while( 1 )
    {
        printf("%s(): Mosq connection status: [%d]", __func__, mqttlink_get_mosq_connection_state());
        if( g_cikis_sinyali != 0 )
        {
            break;
        }

        if( mqttlink_get_mosq_connection_state() == e_mqttlink_mosq_connection_state_connection_true )
        {
	        // -- Example of sending a message --
            char buffer_msg[256];
            i++;
            if( i % 2)
            {
                sprintf(buffer_msg, "%d-test-message-topic: %s .", i, sbj1);
                mqttlink_send_mosq_message(sbj1, buffer_msg, e_mqttlink_mosq_message_storage_flag_state_message_dont_keep);
            }
            else
            {
                sprintf(buffer_msg, "%d-test-message-topic: %s .", i, sbj2);
                mqttlink_send_mosq_message(sbj2, buffer_msg, e_mqttlink_mosq_message_storage_flag_state_message_dont_keep);
            }
        }

        mqttlink_sleep_milisec(1000);
    }

	// Terminate the connection.
    mqttlink_finished_mosq();
    printf("Programdan cikildi.");
    mqttlink_log_close();

    return 0;
}

static void
cik(int sig)
{
    g_cikis_sinyali = sig;
}

static void
message_arrived_notification_function(const char *messege, const char *sbj)
{
    printf("%s(): Gelen mesaj: [%s] - Subject: [%s].\n", __func__, messege, sbj);
}
