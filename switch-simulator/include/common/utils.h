/**
 * @file utils.h
 * @brief Utility functions for switch simulator
 */
#ifndef SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_UTILS_H
#define SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_UTILS_H




#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h"
#include "error_codes.h"

/**
 * @brief Convert MAC address to string representation
 * 
 * @param mac MAC address to convert
 * @param buffer Output buffer (should be at least 18 bytes)
 * @return char* Pointer to buffer containing MAC string
 */
char* mac_to_string(const mac_addr_t *mac, char *buffer);

/**
 * @brief Convert string representation to MAC address
 * 
 * @param str String in format "xx:xx:xx:xx:xx:xx"
 * @param mac Output MAC address
 * @return status_t STATUS_SUCCESS if successful
 */
status_t string_to_mac(const char *str, mac_addr_t *mac);

/**
 * @brief Convert IPv4 address to string representation
 * 
 * @param ipv4 IPv4 address in network byte order
 * @param buffer Output buffer (should be at least 16 bytes)
 * @return char* Pointer to buffer containing IP string
 */
char* ipv4_to_string(ipv4_addr_t ipv4, char *buffer);

/**
 * @brief Convert string representation to IPv4 address
 * 
 * @param str String in format "xxx.xxx.xxx.xxx"
 * @param ipv4 Output IPv4 address
 * @return status_t STATUS_SUCCESS if successful
 */
status_t string_to_ipv4(const char *str, ipv4_addr_t *ipv4);

/**
 * @brief Convert IPv6 address to string representation
 * 
 * @param ipv6 IPv6 address
 * @param buffer Output buffer (should be at least 40 bytes)
 * @return char* Pointer to buffer containing IPv6 string
 */
char* ipv6_to_string(const ipv6_addr_t *ipv6, char *buffer);

/**
 * @brief Convert string representation to IPv6 address
 * 
 * @param str String in IPv6 format
 * @param ipv6 Output IPv6 address
 * @return status_t STATUS_SUCCESS if successful
 */
status_t string_to_ipv6(const char *str, ipv6_addr_t *ipv6);



/**
 * @brief Convert status code to human-readable string
 *
 * @param status Status code to convert
 * @return const char* Human readable string representation of status
 */
const char* status_to_string(status_t status);



/**
 * @brief Compare two MAC addresses
 * 
 * @param mac1 First MAC address
 * @param mac2 Second MAC address
 * @return int Negative if mac1 < mac2, 0 if equal, positive if mac1 > mac2
 */
int mac_compare(const mac_addr_t *mac1, const mac_addr_t *mac2);

/**
 * @brief Check if MAC address is broadcast
 * 
 * @param mac MAC address to check
 * @return bool true if MAC is broadcast (FF:FF:FF:FF:FF:FF)
 */
bool mac_is_broadcast(const mac_addr_t *mac);

/**
 * @brief Check if MAC address is multicast
 * 
 * @param mac MAC address to check
 * @return bool true if MAC is multicast (least significant bit of first byte set)
 */
bool mac_is_multicast(const mac_addr_t *mac);

/**
 * @brief Get current timestamp in milliseconds
 * 
 * @return uint64_t Current timestamp
 */
uint64_t get_timestamp_ms(void);

/**
 * @brief Alias for get_timestamp_ms() function
 * 
 * @return uint64_t Current timestamp in milliseconds
 */
uint64_t get_current_time(void);

/**
 * @brief Calculate CRC32 checksum for data
 * 
 * @param data Input data buffer
 * @param length Length of data in bytes
 * @return uint32_t CRC32 checksum
 */
uint32_t calculate_crc32(const uint8_t *data, size_t length);

/**
 * @brief Memory copy function with bounds checking
 * 
 * @param dst Destination buffer
 * @param dst_size Size of destination buffer
 * @param src Source buffer
 * @param src_size Size to copy from source
 * @return status_t STATUS_SUCCESS if successful
 */
status_t safe_memcpy(void *dst, size_t dst_size, const void *src, size_t src_size);

/**
 * @brief Convert port ID to string name
 * 
 * @param port_id Port identifier
 * @param buffer Output buffer (should be at least 16 bytes)
 * @return char* Pointer to buffer containing port name
 */
char* port_id_to_name(port_id_t port_id, char *buffer);

/**
 * @brief Generate random MAC address
 * 
 * @param mac Output MAC address
 */
void generate_random_mac(mac_addr_t *mac);

/**
 * @brief Parse VLAN range string into VLAN IDs
 * 
 * @param range_str String in format "X" or "X-Y" where X,Y are VLAN IDs
 * @param vlan_ids Output array of VLAN IDs
 * @param max_ids Maximum number of IDs that can be stored in array
 * @param[out] count Number of VLAN IDs parsed
 * @return status_t STATUS_SUCCESS if successful
 */
status_t parse_vlan_range(const char *range_str, vlan_id_t *vlan_ids, 
                         uint32_t max_ids, uint32_t *count);

/**
 * @brief Safely concatenate strings with buffer size checking
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param src Source string to append
 * @return status_t STATUS_SUCCESS if successful
 */
status_t safe_strcat(char *dest, size_t dest_size, const char *src);

/**
 * @brief Check if a string is a valid decimal number
 * 
 * @param str String to check
 * @return bool true if string is a valid number
 */
bool is_valid_number(const char *str);

/**
 * @brief Convert byte array to hexadecimal string
 * 
 * @param data Byte array
 * @param length Length of byte array
 * @param buffer Output buffer (should be at least length*2+1 bytes)
 * @return char* Pointer to buffer containing hex string
 */
char* bytes_to_hex(const uint8_t *data, size_t length, char *buffer);

/**
 * @brief Convert hexadecimal string to byte array
 * 
 * @param hex Hexadecimal string
 * @param data Output byte array
 * @param max_length Maximum length of byte array
 * @param[out] length Actual length of converted data
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hex_to_bytes(const char *hex, uint8_t *data, size_t max_length, size_t *length);

#endif /* SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_UTILS_H */
