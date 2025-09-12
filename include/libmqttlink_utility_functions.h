#ifndef libmqttlink_UTILITY_FUNCTIONS_H
#define libmqttlink_UTILITY_FUNCTIONS_H

#include <stddef.h>


#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        char net_add[64];
        int port;
    } libmqttlink_ntp_server_info;


    /**
    *   This function temporarily suspends the operation for a certain period of time.
    *   @param milisec: Pause time in milliseconds.
    *   @return  None
    **/
    void
    mqttlink_sleep_milisec(unsigned int milisec);


    /**
    *   @param None 
    *   @return Get time as nanosecond. 
    **/
    double 
    mqttlink_get_second_nanosec(void);


    /**
    *   Check if a file exists
    *   @param file_name    
    *   @return None
    **/
    int
    mqttlink_check_if_a_file_exists(const char* file_name); 


    /**
    *   System time.
    *   @param None 
    *   @return Get system time but since at 1970.
    **/
    double
    mqttlink_get_system_time(void);


    /**
    *   Get system date and time.
    *   @param None     
    *   @return Date and system time. =>  03-27-2023  13:28:45.234567
    **/
    int
    mqttlink_get_current_system_time_and_date(char *time_date);


    /**
    *   Get system time diff second remote NTP server
    *   @param server_add_list, list_len    
    *   @return time diff second if err return -1
    **/
    int
    mqttlink_get_ntp_time_difference_second(const libmqttlink_ntp_server_info* ntp_server_list, int ntp_server_list_len);


    /**
    *   Get system default net interface
    *   @param interface name dst
    *   @return if err return -1
    **/
    int
    get_default_interface(char *interface);


    /**
    *   Get system IP add
    *   @param ip address dst
    *   @return if err return -1
    **/   
    int
    get_primary_IP(char* dst);

    /**
    *   Encrypt a file with openssl-aes_256
    *   @param inputFilePath file path to encrypt
    *   @param outputFilePath encrypted output file path
    *   @param key openssl key for encrypt
    *   @param iv openssl iv for encrypt
    *   @return if err return -1 
    **/
    int mqttlink_encrypt_file(const char* inputFilePath, const char* outputFilePath, const unsigned char* key, const unsigned char* iv); 


    /**
    * Encrypts data using AES-256-CBC and saves to files
    * @param input_text Plaintext string to encrypt
    * @param output_bin Path for output encrypted binary file
    * @param output_key Path for output encryption key file
    * @param output_iv Path for output initialization vector file
    * @return 1 on success, 0 on failure
    **/
    int mqttlink_creat_encrypt_text(const char* input_text, const char* output_bin ,const char* output_key, const char* output_iv);

    /**
    * Decrypts a file using AES-256-CBC
    * @param encrypted_file Path to encrypted file as .bin
    * @param key_file Path to key file (32 bytes)
    * @param iv_file Path to IV file (16 bytes)
    * @param decrypted_data Output buffer for decrypted data
    * @param decrypted_len Output length of decrypted data
    * @return 1 on success, -1 on file errors, 0 on decryption failures
    */
    int mqttlink_decrypt_file(const char* encrypted_file, const char* key_file, const char* iv_file, unsigned char** decrypted_data, size_t* decrypted_len);


    char* mqttlink_base64_encode(const unsigned char* input, size_t length);

    int mqttlink_check_basic_auth(const char* auth_header, const char* src_auth); 

#ifdef __cplusplus
}
#endif

#endif // libmqttlink_UTILITY_FUNCTIONS_H
