/**
 * @file types.h
 * @brief Base type definitions for switch simulator
 */

#ifndef SWITCH_SIM_TYPES_H
#define SWITCH_SIM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAC_ADDR_LEN 6

// <old-place
//#define MAX_PORTS               64
//#define MAX_MAC_TABLE_ENTRIES   8192
//#define MAX_PACKET_SIZE         9216  // Support for jumbo frames

//#define MAX_VLANS               4096
//#define VLAN_NAME_MAX_LEN       32
// old-place>


/* Макросы для доступа к отдельным октетам IPv4-адреса */
#define IPV4_OCTET1(ip) (((ip) >> 24) & 0xFF)  /* Первый октет (старший байт) */
#define IPV4_OCTET2(ip) (((ip) >> 16) & 0xFF)  /* Второй октет */
#define IPV4_OCTET3(ip) (((ip) >> 8) & 0xFF)   /* Третий октет */
#define IPV4_OCTET4(ip) ((ip) & 0xFF)          /* Четвертый октет (младший байт) */


typedef enum
{
    // Успешные статусы
    STATUS_SUCCESS = 0,                    /**< Operation completed successfully */

    // Общие ошибки (-1 до -99)
    STATUS_GENERAL_ERROR = -1,             /**< General error */
    STATUS_NOT_INITIALIZED = -2,           /**< Module not initialized */
    STATUS_ALREADY_INITIALIZED = -3,       /**< Module already initialized */
    STATUS_NOT_FOUND = -4,                 /**< Object not found */
    STATUS_INVALID_PARAMETER = -5,         /**< Invalid parameter */
    STATUS_MEMORY_ALLOCATION_FAILED = -6,  /**< Memory allocation error */
    STATUS_RESOURCE_BUSY = -7,             /**< Resource busy */
    STATUS_TIMEOUT = -8,                   /**< Operation timeout */
    STATUS_PERMISSION_DENIED = -9,         /**< Access denied */
    STATUS_UNSUPPORTED_OPERATION = -10,    /**< Operation not supported */
    STATUS_RESOURCE_EXHAUSTED = -11,       /**< Resource exhausted */
    STATUS_RESOURCE_UNAVAILABLE = -12,     /**< Resource unavailable */
    STATUS_RESOURCE_EXCEEDED = -13,        /**< Resource limit exceeded */
    STATUS_NO_MEMORY = -14,                /**< Insufficient memory */
    STATUS_UNKNOWN_ERROR = -15,            /**< Unknown error */
    STATUS_OUT_OF_BOUNDS = -16,            /**< Access beyond buffer or array bounds */
    STATUS_OUT_OF_MEMORY = -17,            /**< Access beyond buffer or array bounds */
//    STATUS_INVALID_PORT = -17,             /**< Invalid port identifier */
//    STATUS_PORT_DOWN = -18,                /**< Port is down */
//    STATUS_HAL_ERROR = -19,                /**< HAL error */
    STATUS_MODULE_NOT_INITIALIZED = -20,   /**< Module not initialized */
    STATUS_INSUFFICIENT_RESOURCES = -21,   /**< Insufficient resources */
    STATUS_ERROR_INVALID_PARAM  = -22,         /**< Invalid param */
    STATUS_ERROR = -23,
    STATUS_FAILURE = -99                  /**< Generic failure */
} status_t;




typedef struct 
{
    uint8_t addr[MAC_ADDR_LEN];  // Использовать MAC_ADDR_LEN вместо 6
} mac_addr_t;

typedef uint32_t ipv4_addr_t;

typedef struct
{
    uint8_t addr[16];
} ipv6_addr_t;

typedef uint16_t port_id_t;

typedef uint16_t vlan_id_t;

typedef uint32_t switch_id_t;

typedef struct {
    uint8_t* data;           /**< Указатель на данные пакета */
    size_t length;           /**< Длина пакета в байтах */
    port_id_t ingress_port;  /**< Входной порт пакета */
    vlan_id_t vlan_id;       /**< VLAN пакета */
} packet_info_t;

typedef status_t (*cli_cmd_handler_t)(int argc, char **argv, char *output, size_t output_len);


#endif   /* SWITCH_SIM_TYPES_H */
