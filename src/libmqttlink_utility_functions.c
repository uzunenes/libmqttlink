#include "../include/libmqttlink_utility_functions.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <string.h>

/**
 * Sleep for given milliseconds.
 */
void libmqttlink_sleep_milisec(unsigned int milisec)
{
    usleep(milisec * 1000);
}

/**
 * Get current system time in seconds (with microsecond precision).
 */
double libmqttlink_get_system_time(void)
{
    struct timeval time;
    if(gettimeofday(&time, NULL))
    {
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

/**
 * Get current monotonic time in seconds (with nanosecond precision).
 */
double libmqttlink_get_second_nanosec(void)
{
    struct timespec second;
    clock_gettime(CLOCK_MONOTONIC, &second);
    return second.tv_sec + second.tv_nsec * 1E-9;
}

/**
 * Check if a file exists.
 * Returns 0 if found, -1 if not found.
 */
int libmqttlink_check_if_a_file_exists(const char* file_name)
{
    if (access(file_name, F_OK) != -1)
        return 0;
    return -1;
}

/**
 * Get current system date and time as string.
 * Returns 0 on success, -1 on error.
 */
int libmqttlink_get_current_system_time_and_date(char* time_date)
{
    struct timeval time;
    time_t curtime;
    #define buffer_len 32
    char buffer[buffer_len] = {0};

    if (gettimeofday(&time, NULL))
        return -1;

    curtime = time.tv_sec;
    strftime(buffer, buffer_len, "%m-%d-%Y %T.", localtime(&curtime));
    snprintf(time_date, 24, "%s%06ld", buffer, time.tv_usec);
    return 0;
}

/**
 * Get primary IP address of default network interface.
 * Returns 0 on success, -1 on error.
 */
int libmqttlink_get_primary_IP(char* dst)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
        return -1;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET && !(ifa->ifa_flags & IN_LOOPBACKNET))
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0)
            {
                strncpy(dst, host, NI_MAXHOST);
                freeifaddrs(ifaddr);
                return 0;
            }
        }
    }
    freeifaddrs(ifaddr);
    return -1;
}

/**
 * Get time difference (in seconds) from NTP server.
 * Returns time difference or -1 on error.
 */
int libmqttlink_get_ntp_time_difference_second(const libmqttlink_ntp_server_info* ntp_server_list, int ntp_server_list_len)
{
    #define NTP_TIMESTAMP_DELTA 2208988800ull
    for (int i = 0; i < ntp_server_list_len; ++i)
    {
        fprintf(stdout, "%s(): Trying getting time ntp server: [%s]. \n", __func__,  ntp_server_list[i].net_add);
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(ntp_server_list[i].port);

        int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockfd < 0)
        {
            fprintf(stdout, "%s(): Socket init error! \n", __func__);
            continue;
        }

        struct hostent *server = gethostbyname(ntp_server_list[i].net_add);
        if (server == NULL)
        {
            fprintf(stdout, "%s(): Host error: [%s]. \n", __func__, ntp_server_list[i].net_add);
            close(sockfd);
            continue;
        }
        fprintf(stdout, "%s(): Server: [%s], port: [%d]. \n", __func__, ntp_server_list[i].net_add, ntp_server_list[i].port);

        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        char buffer[48];
        memset(buffer, 0, sizeof(buffer));
        buffer[0] = 0b11100011;   // LI, Version, Mode
        buffer[1] = 0;            // Stratum
        buffer[2] = 6;            // Polling Interval
        buffer[3] = 0xEC;         // Precision
        buffer[12] = 0x49;
        buffer[13] = 0x4E;
        buffer[14] = 0x49;
        buffer[15] = 0x52;

        if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            fprintf(stdout, "%s(): Sendto error ntp server: [%s]. \n",  __func__, ntp_server_list[i].net_add);
            close(sockfd);
            continue;
        }

        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0) {
            fprintf(stdout, "%s(): Recvfrom error: [%s]. \n",  __func__, ntp_server_list[i].net_add);
            close(sockfd);
            continue;
        }

        unsigned long long *timestamp = (unsigned long long *)&buffer[40];
        *timestamp = ntohl(*timestamp);
        *timestamp -= NTP_TIMESTAMP_DELTA;
        struct timeval tv;
        gettimeofday(&tv, NULL);

        unsigned long long local_time = tv.tv_sec + tv.tv_usec / 1000000;
        int time_difference = (int)(*timestamp - local_time);

        close(sockfd);
        fflush(stdout);
        return time_difference;
    }
    fflush(stdout);
    return -1;
}

/**
 * Encrypt a file using AES-256-CBC.
 * Returns 0 on success, -1 on error.
 */
int libmqttlink_encrypt_file(const char* inputFilePath, const char* outputFilePath, const unsigned char* key, const unsigned char* iv)
{
    FILE* inputFile = fopen(inputFilePath, "rb");
    if (inputFile == NULL) {
        fprintf(stderr, "Failed to open input file: %s\n", inputFilePath);
        return -1;
    }

    FILE* outputFile = fopen(outputFilePath, "wb");
    if (outputFile == NULL) {
        fprintf(stderr, "Failed to open output file: %s\n", outputFilePath);
        fclose(inputFile);
        return -1;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create cipher context\n");
        fclose(inputFile);
        fclose(outputFile);
        return -1;
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        fprintf(stderr, "Failed to initialize encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(inputFile);
        fclose(outputFile);
        return -1;
    }

    unsigned char buffer[4096];
    unsigned char encryptedBuffer[4096 + EVP_MAX_BLOCK_LENGTH];
    int bytesRead = 0;
    int bytesEncrypted = 0;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), inputFile)) > 0) {
        if (EVP_EncryptUpdate(ctx, encryptedBuffer, &bytesEncrypted, buffer, bytesRead) != 1) {
            fprintf(stderr, "Error during encryption\n");
            EVP_CIPHER_CTX_free(ctx);
            fclose(inputFile);
            fclose(outputFile);
            return -1;
        }
        fwrite(encryptedBuffer, 1, bytesEncrypted, outputFile);
    }

    if (EVP_EncryptFinal_ex(ctx, encryptedBuffer, &bytesEncrypted) != 1) {
        fprintf(stderr, "Error finalizing encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(inputFile);
        fclose(outputFile);
        return -1;
    }
    fwrite(encryptedBuffer, 1, bytesEncrypted, outputFile);

    EVP_CIPHER_CTX_free(ctx);
    fclose(inputFile);
    fclose(outputFile);

    return 0;
}

/**
 * Encrypt a text string and write encrypted data, key, and IV to files.
 * Returns 1 on success, 0 on error.
 */
int libmqttlink_create_encrypt_text(const char* input_text, const char* output_bin, const char* output_key, const char* output_iv) 
{
    unsigned char key[32];
    unsigned char iv[16];
    unsigned char* plaintext = (unsigned char*)input_text;
    int plaintext_len = strlen(input_text);
    int ciphertext_len = 0;
    
    if (RAND_bytes(key, sizeof(key)) != 1 || RAND_bytes(iv, sizeof(iv)) != 1) {
        fprintf(stderr, "Failed to generate random values\n");
        return 0;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create encryption context\n");
        return 0;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        fprintf(stderr, "Failed to initialize encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    unsigned char* ciphertext = (unsigned char*)malloc(plaintext_len + AES_BLOCK_SIZE);
    if (!ciphertext) {
        fprintf(stderr, "Memory allocation failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    int len;
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        fprintf(stderr, "Encryption failed\n");
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        fprintf(stderr, "Failed to finalize encryption\n");
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    ciphertext_len += len;

    FILE* f_enc = fopen(output_bin, "wb");
    FILE* f_key = fopen(output_key, "wb");
    FILE* f_iv = fopen(output_iv, "wb");

    if (!f_enc || !f_key || !f_iv) {
        fprintf(stderr, "Failed to open output files\n");
        if (f_enc) fclose(f_enc);
        if (f_key) fclose(f_key);
        if (f_iv) fclose(f_iv);
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    fwrite(ciphertext, 1, ciphertext_len, f_enc);
    fwrite(key, 1, sizeof(key), f_key);
    fwrite(iv, 1, sizeof(iv), f_iv);

    fclose(f_enc);
    fclose(f_key);
    fclose(f_iv);
    free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);

    printf("Encryption completed. Output files: %s, %s, %s\n", 
           output_bin, output_key, output_iv);
    return 1;
}

/**
 * Decrypt a file using AES-256-CBC.
 * Returns 1 on success, -1 on error.
 */
int libmqttlink_decrypt_file(const char* encrypted_file, const char* key_file, const char* iv_file, unsigned char** decrypted_data, size_t* decrypted_len) 
{
    FILE* key_fp = fopen(key_file, "rb");
    if (!key_fp) {
        fprintf(stderr, "Error: Could not open key file %s\n", key_file);
        return -1;
    }
    unsigned char key[32];
    if (fread(key, 1, 32, key_fp) != 32) {
        fprintf(stderr, "Error: Could not read 32 bytes from key file %s\n", key_file);
        fclose(key_fp);
        return -1;
    }
    fclose(key_fp);

    FILE* iv_fp = fopen(iv_file, "rb");
    if (!iv_fp) {
        fprintf(stderr, "Error: Could not open IV file %s\n", iv_file);
        return -1;
    }
    unsigned char iv[16];
    if (fread(iv, 1, 16, iv_fp) != 16) {
        fprintf(stderr, "Error: Could not read 16 bytes from IV file %s\n", iv_file);
        fclose(iv_fp);
        return -1;
    }
    fclose(iv_fp);

    FILE* enc_fp = fopen(encrypted_file, "rb");
    if (!enc_fp) {
        fprintf(stderr, "Error: Could not open encrypted file %s\n", encrypted_file);
        return -1;
    }
    fseek(enc_fp, 0, SEEK_END);
    long ciphertext_len = ftell(enc_fp);
    if (ciphertext_len < 0) {
        fprintf(stderr, "Error: Could not determine file size for %s\n", encrypted_file);
        fclose(enc_fp);
        return -1;
    }
    fseek(enc_fp, 0, SEEK_SET);

    unsigned char* ciphertext = (unsigned char*)malloc(ciphertext_len);
    if (!ciphertext) {
        fprintf(stderr, "Error: Memory allocation failed for ciphertext buffer\n");
        fclose(enc_fp);
        return -1;
    }
    if (fread(ciphertext, 1, ciphertext_len, enc_fp) != (size_t)ciphertext_len) {
        fprintf(stderr, "Error: Could not read complete encrypted data from %s\n", encrypted_file);
        free(ciphertext);
        fclose(enc_fp);
        return -1;
    }
    fclose(enc_fp);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error: Could not create EVP cipher context\n");
        free(ciphertext);
        return -1;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        fprintf(stderr, "Error: Could not initialize decryption\n");
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    *decrypted_data = (unsigned char*)malloc(ciphertext_len + AES_BLOCK_SIZE);
    if (!*decrypted_data) {
        fprintf(stderr, "Error: Memory allocation failed for decrypted data buffer\n");
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int len = 0, plaintext_len = 0;
    if (1 != EVP_DecryptUpdate(ctx, *decrypted_data, &len, ciphertext, ciphertext_len)) {
        fprintf(stderr, "Error: Decryption failed during update\n");
        free(ciphertext);
        free(*decrypted_data);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, *decrypted_data + len, &len)) {
        fprintf(stderr, "Error: Decryption failed during finalization\n");
        free(ciphertext);
        free(*decrypted_data);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;
    *decrypted_len = plaintext_len;

    *decrypted_data = (unsigned char*)realloc(*decrypted_data, *decrypted_len);
    if (!*decrypted_data) {
        fprintf(stderr, "Error: Memory reallocation failed for decrypted data\n");
        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (*decrypted_len > 0)
        (*decrypted_data)[*decrypted_len] = '\0';  

    free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

/**
 * Check HTTP Basic Auth header against username:password.
 * Returns 1 if matches, 0 if not.
 */
int libmqttlink_check_basic_auth(const char* auth_header, const char* src_auth) 
{
    size_t auth_len = strnlen(src_auth, 64);
    if (auth_len == 0 || strchr(src_auth, ':') == NULL) {
        fprintf(stderr, "Invalid src_auth format. Expected 'username:password'\n");
        return 0;
    }

    char* encoded = libmqttlink_base64_encode((const unsigned char*)src_auth, auth_len);
    if (!encoded) {
        return 0;
    }

    size_t expected_len = strlen("Basic ") + strlen(encoded) + 1;
    char* expected_auth = (char*)malloc(expected_len);
    if (!expected_auth) {
        free(encoded);
        return 0;
    }

    snprintf(expected_auth, expected_len, "Basic %s", encoded);

    int result = (strcmp(expected_auth, auth_header) == 0);

    free(encoded);
    free(expected_auth);

    return result;
}

/**
 * Encode data to Base64 string.
 * Returns pointer to encoded string (must be freed by caller).
 */
char* libmqttlink_base64_encode(const unsigned char* input, size_t length) 
{
    const char encode_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_length = 4 * ((length + 2) / 3); 
    char* encoded = (char*)malloc(output_length + 1); 
    if (encoded == NULL) return NULL;

    int val = 0, valb = -6;
    size_t pos = 0;

    for (size_t i = 0; i < length; i++) {
        val = (val << 8) + input[i];
        valb += 8;
        while (valb >= 0) {
            encoded[pos++] = encode_table[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }

    if (valb > -6) {
        encoded[pos++] = encode_table[((val << 8) >> (valb + 8)) & 0x3F];
    }

    while (pos % 4) {
        encoded[pos++] = '=';
    }

    encoded[pos] = '\0'; 
    return encoded;
}