#ifndef LIBMQTTLINK_UTILITY_FUNCTIONS_H
#define LIBMQTTLINK_UTILITY_FUNCTIONS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char net_add[64];
    int port;
} libmqttlink_ntp_server_info;

/**
 * Sleep for a given number of milliseconds.
 * @param milisec Milliseconds to sleep.
 */
void libmqttlink_sleep_milisec(unsigned int milisec);

/**
 * Get current monotonic time in seconds (nanosecond precision).
 * @return Time in seconds.
 */
double libmqttlink_get_second_nanosec(void);

/**
 * Check if a file exists.
 * @param file_name File path.
 * @return 0 if exists, -1 if not.
 */
int libmqttlink_check_if_a_file_exists(const char* file_name);

/**
 * Get current system time in seconds since epoch.
 * @return Time in seconds.
 */
double libmqttlink_get_system_time(void);

/**
 * Get current system date and time as string.
 * @param time_date Output buffer for date/time string.
 * @return 0 on success, -1 on error.
 */
int libmqttlink_get_current_system_time_and_date(char *time_date);

/**
 * Get time difference (in seconds) from remote NTP server.
 * @param ntp_server_list List of NTP servers.
 * @param ntp_server_list_len Length of server list.
 * @return Time difference in seconds, -1 on error.
 */
int libmqttlink_get_ntp_time_difference_second(const libmqttlink_ntp_server_info* ntp_server_list, int ntp_server_list_len);

/**
 * Get system default network interface name.
 * @param interface Output buffer for interface name.
 * @return 0 on success, -1 on error.
 */
int libmqttlink_get_default_interface(char *interface);

/**
 * Get primary IP address of default network interface.
 * @param dst Output buffer for IP address.
 * @return 0 on success, -1 on error.
 */
int libmqttlink_get_primary_IP(char* dst);

/**
 * Encrypt a file using AES-256-CBC.
 * @param inputFilePath Path to input file.
 * @param outputFilePath Path to output encrypted file.
 * @param key Encryption key.
 * @param iv Initialization vector.
 * @return 0 on success, -1 on error.
 */
int libmqttlink_encrypt_file(const char* inputFilePath, const char* outputFilePath, const unsigned char* key, const unsigned char* iv);

/**
 * Encrypt a text string and save encrypted data, key, and IV to files.
 * @param input_text Plaintext string.
 * @param output_bin Output encrypted binary file path.
 * @param output_key Output key file path.
 * @param output_iv Output IV file path.
 * @return 1 on success, 0 on error.
 */
int libmqttlink_creat_encrypt_text(const char* input_text, const char* output_bin, const char* output_key, const char* output_iv);

/**
 * Decrypt a file using AES-256-CBC.
 * @param encrypted_file Path to encrypted file.
 * @param key_file Path to key file.
 * @param iv_file Path to IV file.
 * @param decrypted_data Output pointer for decrypted data.
 * @param decrypted_len Output length of decrypted data.
 * @return 1 on success, -1 on error.
 */
int libmqttlink_decrypt_file(const char* encrypted_file, const char* key_file, const char* iv_file, unsigned char** decrypted_data, size_t* decrypted_len);

/**
 * Encode data to Base64 string.
 * @param input Input data.
 * @param length Length of input data.
 * @return Pointer to encoded string (must be freed by caller).
 */
char* libmqttlink_base64_encode(const unsigned char* input, size_t length);

/**
 * Check HTTP Basic Auth header against username:password.
 * @param auth_header Authorization header string.
 * @param src_auth Username:password string.
 * @return 1 if matches, 0 if not.
 */
int libmqttlink_check_basic_auth(const char* auth_header, const char* src_auth);

#ifdef __cplusplus
}
#endif

#endif // LIBMQTTLINK_UTILITY_FUNCTIONS_H