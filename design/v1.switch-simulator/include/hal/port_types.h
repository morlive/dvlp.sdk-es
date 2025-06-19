/**
 * @file port_types.h
 * @brief Common port types and definitions used across hardware abstraction layer
 */

#ifndef SWITCH_SIM_PORT_TYPES_H
#define SWITCH_SIM_PORT_TYPES_H

#include "../common/types.h"
#include "driver.h"  /* Добавляем включение драйверных типов */


// Идентификаторы портов - специальные значения

// broadcast по всем портам
#define PORT_ID_BROADCAST     ((port_id_t)~(port_id_t)0)        // 0xFFFF

// CPU-порт / internal
#define PORT_ID_INTERNAL      ((port_id_t)(~(port_id_t)0 - 1))  // 0xFFFE

// недействительный порт
#define PORT_ID_INVALID       ((port_id_t)(~(port_id_t)0 - 2))  // 0xFFFD

#define PORT_ID_ALL           PORT_ID_BROADCAST  // Все порты (широковещательная рассылка)


/**
 * @brief Maximum number of physical ports supported
 */
#define   MAX_PORTS   64

/**
 * @brief Port identifier type
 */
typedef   uint16_t    port_id_t;  // Используем 16-битный тип

/**
 * @brief Port duplex mode enumeration
 */
typedef enum {
    PORT_SPEED_10M      = 10,         /**< 10 Mbps */
    PORT_SPEED_100M     = 100,        /**< 100 Mbps */
    PORT_SPEED_1G       = 1000,       /**< 1 Gbps */
    PORT_SPEED_10G      = 10000,      /**< 10 Gbps */
    PORT_SPEED_25G      = 25000,      /**< 25 Gbps */
    PORT_SPEED_40G      = 40000,      /**< 40 Gbps */
    PORT_SPEED_100G     = 100000,     /**< 100 Gbps */
    PORT_SPEED_UNKNOWN  = 0           /**< Unknown/unspecified speed */
} port_speed_t;

/**
 * @brief Port duplex mode enumeration
 */
typedef enum {
    PORT_DUPLEX_HALF    = 0,          /**< Half duplex */
    PORT_DUPLEX_FULL,                 /**< Full duplex */
    PORT_DUPLEX_UNKNOWN               /**< Unknown/unspecified duplex */
} port_duplex_t;

/**
 * @brief Port operational state enumeration
 */
typedef enum {
    PORT_STATE_DOWN     = 0,          /**< Port is down */
    PORT_STATE_UP,                    /**< Port is up and operational */
    PORT_STATE_LEARNING,              /**< Port is in learning state (STP) */
    PORT_STATE_FORWARDING,            /**< Port is in forwarding state (STP) */
    PORT_STATE_BLOCKING,              /**< Port is in blocking state (STP) */
    PORT_STATE_TESTING,               /**< Port is in testing mode */
    PORT_STATE_UNKNOWN                /**< Port state is unknown */
} port_state_t;

/**
 * @brief Port type enumeration
 */
typedef enum {
    PORT_TYPE_PHYSICAL  = 0,          /**< Physical port */
    PORT_TYPE_LAG,                    /**< Link Aggregation Group */
    PORT_TYPE_LOOPBACK,               /**< Loopback interface */
    PORT_TYPE_CPU                     /**< CPU port */
} port_type_t;

/**
 * @brief Port operation mode at hardware level
 */
typedef enum {
    PORT_MODE_NORMAL = 0,      /**< Normal operation mode */
    PORT_MODE_LOOPBACK,        /**< Internal loopback testing mode */
    PORT_MODE_MONITOR_SRC,     /**< Port mirroring source */
    PORT_MODE_MONITOR_DST,     /**< Port mirroring destination */
    PORT_MODE_DIAGNOSTIC       /**< Hardware diagnostic mode */
} port_mode_t;

/**
 * @brief Port configuration structure
 */
typedef struct {
    bool          admin_state;        /**< Administrative state (true = enabled) */
    port_speed_t  speed;              /**< Port speed */
    port_duplex_t duplex;             /**< Duplex mode */
    bool          auto_neg;           /**< Auto-negotiation enabled */
    bool          flow_control;       /**< Flow control enabled */
    uint16_t      mtu;                /**< Maximum Transmission Unit */
    vlan_id_t     pvid;               /**< Port VLAN ID */
    //void*         driver;             /**< Driver reference */

    driver_handle_t driver;        /* Driver handle for this port */
    void *driver_private;          /* Driver-specific private data */
    uint32_t flags;               /* Port-specific flags */

    port_mode_t   mode;               /**< Port mode */
} port_config_t;


#endif /* SWITCH_SIM_PORT_TYPES_H */


