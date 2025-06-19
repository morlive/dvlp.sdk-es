#ifndef SWITCH_SIM_ERROR_CODES_H
#define SWITCH_SIM_ERROR_CODES_H

#include "types.h"

/**
 * @brief Three-dimensional error handling architecture:
 * 1. Abstraction levels (ranges):
 *    - STATUS_* (general statuses): from -1 to -99
 *    - L2 errors: from -100 to -199
 *    - L3 errors: from -200 to -299
 *    - Drivers and BSP: from -300 to -399
 *    - HAL: from -400 to -499
 *    - SAI: from -500 to -599
 *    - CLI/API: from -600 to -699
 * 
 * 2. Subsystems: Detailed within each range
 * 
 * 3. Brand abstraction: Separate layer for emulating different brands
 */


#define FRAGMENT_REASSEMBLY_TIMEOUT ERROR_FRAGMENT_REASSEMBLY_TIMEOUT   /**< Alias for ERROR_FRAGMENT_REASSEMBLY_TIMEOUT */

/**
 * @brief Type for storing statuses and error codes
 */
//typedef int status_t;

/**
 * @brief System component identifiers
 */
typedef enum 
{
    COMPONENT_GENERAL = 0,     /**< General system level */
    COMPONENT_HAL,             /**< Hardware Abstraction Layer */
    COMPONENT_BSP,             /**< Board Support Package */
    COMPONENT_L2,              /**< L2 functionality */
    COMPONENT_L3,              /**< L3 functionality */
    COMPONENT_SAI,             /**< Switch Abstraction Interface */
    COMPONENT_CLI,             /**< Command Line Interface */
    COMPONENT_DRIVER,          /**< Hardware drivers */
    COMPONENT_MAX              /**< Maximum component identifier */
} component_id_t;


/*===========================================================================*/
/* 0. COMMON ERRORS (can be used across all components)                      */
/*===========================================================================*/
#define ERROR_NONE                          -50     /**< No error */
#define ERROR_INTERNAL                      -51     /**< Internal error - critical failure */
#define ERROR_OUT_OF_MEMORY                 -52     /**< Critical memory failure */
#define ERROR_IN_PROGRESS                   -53

/* Алиас для “not initialized” */
#ifndef ERROR_NOT_INITIALIZED
#define ERROR_NOT_INITIALIZED               STATUS_NOT_INITIALIZED  /**< Module not initialized */
#endif

/* Алиас для “invalid parameter” */
#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER            STATUS_INVALID_PARAMETER  /**< Invalid function parameter */
#endif

/* Алиас для “invalid param” */
#ifndef ERROR_INVALID_PARAM
#define ERROR_INVALID_PARAM            STATUS_ERROR_INVALID_PARAM  /**< Invalid function param */
#endif

/*===========================================================================*/
/* 1. GENERAL STATUSES (-1 to -99)                                           */
/*===========================================================================*/
// #define STATUS_SUCCESS                    0      /**< Successful execution */
// #define STATUS_GENERAL_ERROR             -1      /**< General error */
// #define STATUS_NOT_INITIALIZED           -2      /**< Module not initialized */
// #define STATUS_ALREADY_INITIALIZED       -3      /**< Module already initialized */
// #define STATUS_NOT_FOUND                 -4      /**< Object not found */
// #define STATUS_INVALID_PARAMETER         -5      /**< Invalid parameter */
// #define STATUS_MEMORY_ALLOCATION_FAILED  -6      /**< Memory allocation error */
// #define STATUS_RESOURCE_BUSY             -7      /**< Resource busy */
// #define STATUS_TIMEOUT                   -8      /**< Operation timeout */
// #define STATUS_PERMISSION_DENIED         -9      /**< Access denied */
// #define STATUS_UNSUPPORTED_OPERATION     -10     /**< Operation not supported */
// #define STATUS_RESOURCE_EXHAUSTED        -11     /**< Resource exhausted */
// #define STATUS_RESOURCE_UNAVAILABLE      -12     /**< Resource unavailable */
// #define STATUS_RESOURCE_EXCEEDED         -13     /**< Resource limit exceeded */
// #define STATUS_NO_MEMORY                 -14     /**< Insufficient memory */
// #define STATUS_UNKNOWN_ERROR             -15     /**< Unknown error */
// #define STATUS_OUT_OF_BOUNDS             -16     /**< Access beyond buffer or array bounds */

#ifndef STATUS_INVALID_PACKET
#define STATUS_INVALID_PACKET              -2200    /**< Invalid packet */
#endif

#ifndef STATUS_INVALID_PORT
#define STATUS_INVALID_PORT                 -80     /**< Invalid port identifier */
#endif

#ifndef STATUS_PORT_DOWN
#define STATUS_PORT_DOWN                    -81     /**< Port is down */
#endif

#ifndef STATUS_HAL_ERROR
#define STATUS_HAL_ERROR                    -82     /**< HAL error */
#endif

// #define STATUS_MODULE_NOT_INITIALIZED    -83     /**< Module not initialized */
// #define STATUS_INSUFFICIENT_RESOURCES    -84     /**< Insufficient resources */

#ifndef STATUS_ALREADY_EXISTS
#define STATUS_ALREADY_EXISTS   ERROR_MAC_ENTRY_EXISTS   /**< Resource already exists (MAC learning) */
#endif

#ifndef STATUS_TABLE_FULL
#define STATUS_TABLE_FULL       ERROR_MAC_TABLE_FULL     /**< Table is full (MAC learning) */
#endif



/*===========================================================================*/
/* 2. L2 LAYER ERRORS (-100 to -199)                                         */
/*===========================================================================*/
#define ERROR_L2_BASE                       -100
#define ERROR_MAC_TABLE_FULL                -101    /**< MAC address table is full */
#define ERROR_MAC_ENTRY_EXISTS              -108    /**< MAC learning entry already exists */

#define ERROR_VLAN_NOT_FOUND                -102    /**< VLAN not found */
#define ERROR_VLAN_ALREADY_EXISTS           -103    /**< VLAN already exists */
#define ERROR_PORT_NOT_IN_VLAN              -104    /**< Port does not belong to VLAN */

#define ERROR_STP_INVALID_STATE             -105    /**< Invalid STP state */
#define ERROR_MAC_INVALID                   -106    /**< Invalid MAC address */
#define ERROR_L2_PACKET_MALFORMED           -107    /**< Malformed L2 packet */

/*===========================================================================*/
/* 3. L3 LAYER ERRORS (-200 to -299)                                         */
/*===========================================================================*/
#define ERROR_L3_BASE                       -200
#define ERROR_PACKET_TOO_SHORT              -201    /**< Packet too short for parsing */
#define ERROR_UNSUPPORTED_PROTOCOL          -202    /**< Protocol not supported */
#define ERROR_PACKET_MALFORMED              -203    /**< Malformed packet */
#define ERROR_PACKET_TOO_BIG                -204    /**< Packet too large */
#define ERROR_INVALID_HEADER                -205    /**< Invalid header */
#define ERROR_INVALID_CHECKSUM              -206    /**< Invalid checksum */
#define ERROR_INVALID_OPTION                -207    /**< Invalid option */
#define ERROR_CANNOT_FRAGMENT               -208    /**< Cannot fragment packet */
#define ERROR_MTU_TOO_SMALL                 -209    /**< MTU too small */
#define ERROR_NOT_IMPLEMENTED               -210    /**< Functionality not implemented */
#define ERROR_ARP_PENDING                   -211    /**< ARP request pending */
#define ERROR_NULL_POINTER                  -212    /**< Null pointer */
#define ERROR_FRAGMENT_REASSEMBLY_TIMEOUT   -213    /**< Fragment reassembly timeout */
#define ERROR_PACKET_OPERATION_FAILED       -214    /**< Packet operation failed */
#define ERROR_REASSEMBLY_IN_PROGRESS        -215    /**< Packet reassembly in progress */
#define ERROR_TTL_EXCEEDED                  -216    /**< TTL value exceeded */
#define ERROR_ENTRY_NOT_FOUND               -217    /**< Entry not found */
#define ERROR_PENDING_RESOLUTION            -218    /**< Resolution in progress */
#define ERROR_INVALID_STATE                 -219    /**< Invalid state */
#define ERROR_INVALID_PACKET                -220    /**< Invalid packet */
#define ERROR_TTL_EXPIRED                   -221    /**< TTL expired */
#define ERROR_NO_ROUTE                      -222    /**< Route not found */
#define ERROR_INVALID_PORT                  -223    /**< Invalid port */
#define ERROR_MEMORY_ALLOCATION_FAILED      -224
#define ERROR_PACKET_ALLOCATION_FAILED      -225

/*===========================================================================*/
/* 4. DRIVER AND BSP ERRORS (-300 to -399)                                   */
/*===========================================================================*/
#define ERROR_DRIVER_BASE                   -300
#define ERROR_DRIVER_INIT_FAILED            -301    /**< Driver initialization failed */
#define ERROR_DEVICE_NOT_FOUND              -302    /**< Device not found */
#define ERROR_IO_ERROR                      -303    /**< I/O error */
#define ERROR_BSP_CONFIG_INVALID            -304    /**< Invalid BSP configuration */

/*===========================================================================*/
/* 5. HAL ERRORS (-400 to -499)                                              */
/*===========================================================================*/
#define ERROR_HAL_BASE                      -400
#define ERROR_HAL_NOT_INITIALIZED           -401    /**< HAL not initialized */
#define ERROR_HAL_ALREADY_INITIALIZED       -402    /**< HAL already initialized */
#define ERROR_HAL_OPERATION_FAILED          -403    /**< HAL operation failed */

/*===========================================================================*/
/* 6. SAI ERRORS (-500 to -599)                                              */
/*===========================================================================*/
#define ERROR_SAI_BASE                      -500
#define ERROR_SAI_INIT_FAILED               -501    /**< SAI initialization failed */
#define ERROR_SAI_ATTRIBUTE_INVALID         -502    /**< Invalid SAI attribute */

/*===========================================================================*/
/* 7. CLI/API ERRORS (-600 to -699)                                          */
/*===========================================================================*/
#define ERROR_CLI_BASE                      -600
#define ERROR_CLI_PARSE_FAILED              -601    /**< Command parsing failed */
#define ERROR_CLI_COMMAND_NOT_FOUND         -602    /**< Command not found */
#define ERROR_API_INVALID_REQUEST           -603    /**< Invalid API request */

/**
 * @brief Function for creating a combined error code
 * @param component Component identifier
 * @param error Error code
 * @return Combined error code
 */
#define MAKE_ERROR_CODE(component, error) \
    ((uint32_t)(((component) & 0xFF) << 16) | ((error) & 0xFFFF))

/**
 * @brief Function for extracting component identifier from an error code
 * @param error_code Error code
 * @return Component identifier
 */
#define GET_ERROR_COMPONENT(error_code) \
    ((component_id_t)(((error_code) >> 16) & 0xFF))

/**
 * @brief Function for extracting error code from a combined code
 * @param error_code Combined error code
 * @return Error code
 */
#define GET_ERROR_CODE(error_code) \
    ((uint16_t)((error_code) & 0xFFFF))

/**
 * @brief Converting error to status
 * @param error_code Error code
 * @return Status
 */
status_t error_to_status(uint32_t error_code);

/**
 * @brief Get string representation of an error code
 * @param error_code Error code
 * @return String representation
 */
const char* error_to_string(uint32_t error_code);


/* Aliases for compatibility */
#define STATUS_NOT_SUPPORTED    STATUS_UNSUPPORTED_OPERATION


#endif /* SWITCH_SIM_ERROR_CODES_H */

