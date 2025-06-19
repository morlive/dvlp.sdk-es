/**
 * @file port.h
 * @brief Port management interface for switch simulator
 *
 * This file defines the interface for managing switch ports, including initialization,
 * configuration, statistics, and packet processing.
 *
 * @author Switch Simulator Team
 */

#ifndef SWITCH_SIM_PORT_H
#define SWITCH_SIM_PORT_H

#include "../common/types.h"
#include "../common/error_codes.h"
#include "../common/port_stats.h"
#include "port_types.h"  // Добавляем включение нового файла
#include "packet.h"


// #define PORT_ID_INVALID_INTERNAL (-1)
//// Если PORT_ID_INVALID уже определена, добавьте после неё:
//// #define PORT_ID_ALL 0xFFFFFFFF  /**< Special value representing all ports */

//// typedef uint16_t port_id_t;  // Используем 16-битный тип
//// #define PORT_ID_INVALID ((port_id_t)(~(port_id_t)0 - 1))    // Предпоследнее возможное значение типа
//// #define PORT_ID_ALL     ((port_id_t)(~(port_id_t)0))        // Максимальное возможное значение для типа

///// broadcast по всем портам
///#define PORT_ID_BROADCAST     ((port_id_t)~(port_id_t)0)        // 0xFFFF
///
///// CPU-порт / internal
///#define PORT_ID_INTERNAL      ((port_id_t)(~(port_id_t)0 - 1))  // 0xFFFE
///
///// недействительный порт
///#define PORT_ID_INVALID       ((port_id_t)(~(port_id_t)0 - 2))  // 0xFFFD
///
///#define PORT_ID_ALL           PORT_ID_BROADCAST  // Все порты (широковещательная рассылка)
///
////**
/// * @brief Port identifier type
/// */
///typedef uint16_t port_id_t;  // Используем 16-битный тип
///
////**
/// * @brief Maximum number of physical ports supported
/// */
///#define MAX_PORTS 64
///
////**
/// * @brief Port speed definitions | Port duplex mode enumeration
/// */
///typedef enum {
///    PORT_SPEED_10M = 10,         /**< 10 Mbps */
///    PORT_SPEED_100M = 100,       /**< 100 Mbps */
///    PORT_SPEED_1G = 1000,        /**< 1 Gbps */
///    PORT_SPEED_10G = 10000,      /**< 10 Gbps */
///    PORT_SPEED_25G = 25000,      /**< 25 Gbps */
///    PORT_SPEED_40G = 40000,      /**< 40 Gbps */
///    PORT_SPEED_100G = 100000,    /**< 100 Gbps */
///    PORT_SPEED_UNKNOWN = 0       /**< Unknown/unspecified speed */
///} port_speed_t;
///
////**
/// * @brief Port duplex mode enumeration
/// */
///typedef enum {
///    PORT_DUPLEX_HALF = 0,    /**< Half duplex */
///    PORT_DUPLEX_FULL,        /**< Full duplex */
///    PORT_DUPLEX_UNKNOWN      /**< Unknown/unspecified duplex */
///} port_duplex_t;
///
////**
/// * @brief Port operational state enumeration
/// */
///typedef enum {
///    PORT_STATE_DOWN = 0,     /**< Port is down */
///    PORT_STATE_UP,           /**< Port is up and operational */
///
///    PORT_STATE_LEARNING,     /**< Port is in learning state (STP) */
///    PORT_STATE_FORWARDING,   /**< Port is in forwarding state (STP) */
///    PORT_STATE_BLOCKING,     /**< Port is in blocking state (STP) */
///
///    PORT_STATE_TESTING,      /**< Port is in testing mode */
///    PORT_STATE_UNKNOWN       /**< Port state is unknown */
///} port_state_t;
///
////**
/// * @brief Port type
/// */
///typedef enum {
///    PORT_TYPE_PHYSICAL = 0,  /**< Physical port */
///    PORT_TYPE_LAG,           /**< Link Aggregation Group */
///    PORT_TYPE_LOOPBACK,      /**< Loopback interface */
///    PORT_TYPE_CPU            /**< CPU port */
///} port_type_t;
///
//////**
///// * @brief Port statistics counters
///// */
/////typedef struct {
/////    uint64_t rx_packets;     /**< Received packets */
/////    uint64_t tx_packets;     /**< Transmitted packets */
/////    uint64_t rx_bytes;       /**< Received bytes */
/////    uint64_t tx_bytes;       /**< Transmitted bytes */
/////    uint64_t rx_errors;      /**< Receive errors */
/////    uint64_t tx_errors;      /**< Transmit errors */
/////    uint64_t rx_drops;       /**< Dropped received packets */
/////    uint64_t tx_drops;       /**< Dropped packets during transmission */
/////    uint64_t rx_unicast;     /**< Received unicast packets */
/////    uint64_t rx_multicast;   /**< Received multicast packets */
/////    uint64_t rx_broadcast;   /**< Received broadcast packets */
/////    uint64_t tx_unicast;     /**< Transmitted unicast packets */
/////    uint64_t tx_multicast;   /**< Transmitted multicast packets */
/////    uint64_t tx_broadcast;   /**< Transmitted broadcast packets */
/////} port_stats_t;
///
////**
/// * @brief Port configuration
/// */
///typedef struct {
///    bool admin_state;        /**< Administrative state (true = enabled) */
///    port_speed_t speed;      /**< Port speed */
///    port_duplex_t duplex;    /**< Duplex mode */
///    bool auto_neg;           /**< Auto-negotiation enabled */
///    bool flow_control;       /**< Flow control enabled */
///    uint16_t mtu;            /**< Maximum Transmission Unit */
///    vlan_id_t pvid;          /**< Port VLAN ID */
///} port_config_t;

/**
 * @brief Port information
 */
typedef struct {
    port_id_t id;            /**< Port identifier */
    port_type_t type;        /**< Port type */
    char name[32];           /**< Port name */
    port_config_t config;    /**< Current configuration */
    port_state_t state;      /**< Operational state */
    port_stats_t stats;      /**< Statistics counters */
    mac_addr_t mac_addr;     /**< MAC address associated with port */
} port_info_t;

/**
 * @brief Initialize port subsystem
 * 
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_init(void);

/**
 * @brief Shutdown port subsystem
 * 
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_shutdown(void);

/**
 * @brief Get port information
 * 
 * @param port_id Port identifier
 * @param[out] info Port information structure to fill
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_info(port_id_t port_id, port_info_t *info);

/**
 * @brief Set port configuration
 * 
 * @param port_id Port identifier
 * @param config Port configuration to apply
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_set_config(port_id_t port_id, const port_config_t *config);

/**
 * @brief Get current port configuration
 *
 * @param port_id Port identifier
 * @param[out] config Pointer to store port configuration
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_get_config(port_id_t port_id, port_config_t *config);

/**
 * @brief Set port administrative state
 * 
 * @param port_id Port identifier
 * @param admin_up True to set port administratively up, false for down
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_set_admin_state(port_id_t port_id, bool admin_up);

/**
 * @brief Enable a port (administrative state up)
 *
 * @param port_id Port identifier
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_enable(port_id_t port_id);

/**
 * @brief Disable a port (administrative state down)
 *
 * @param port_id Port identifier
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_disable(port_id_t port_id);

/**
 * @brief Get port statistics
 * 
 * @param port_id Port identifier
 * @param[out] stats Statistics structure to fill
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_stats(port_id_t port_id, port_stats_t *stats);

/**
 * @brief Clear port statistics counters
 * 
 * @param port_id Port identifier
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_clear_stats(port_id_t port_id);

/**
 * @brief Get total number of ports in the system
 *
 * @param[out] count Pointer to store port count
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_count(uint32_t *count);

/**
 * @brief Возвращает общее число портов, включая CPU-порт
 */
/**
 * @brief Get the total number of ports (including inactive)
 *
 * @param[out] count Pointer to store the total port count
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_get_total_count(uint32_t *count); // ← Добавить здесь

/**
 * @brief Возвращает идентификатор CPU-порта
 */
/**
 * @brief Get the CPU port identifier
 *
 * @return Port identifier for the CPU port
 */
port_id_t port_cpu_id(void);

/**
 * @brief Get list of all port IDs
 *
 * @param[out] port_ids Array to store port IDs
 * @param[in,out] count In: max array size; Out: actual number of ports
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_list(port_id_t *port_ids, uint32_t *count);

/**
 * @brief Check if port ID is valid
 *
 * @param port_id Port identifier to check
 * @return bool True if the port ID is valid, false otherwise
 */
bool port_is_valid(port_id_t port_id);

/**
 * @brief Check if a port is operationally up
 *
 * @param port_id Port identifier
 * @return true if port is up, false otherwise
 */
bool port_is_up(port_id_t port_id);

/**
 * @brief Get port operational state
 *
 * @param port_id Port identifier
 * @param[out] state Pointer to store the port state
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_state(port_id_t port_id, port_state_t *state);


/**
 * @brief Set the operational state of a port
 *
 * @param port_id Port identifier
 * @param state New port state
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_set_state(port_id_t port_id, port_state_t state);

/**
 * @brief Process a received packet from a port
 *
 * This function is called when a packet is received on a port
 * and needs to be processed by the switch.
 *
 * @param port_id Port identifier where the packet was received
 * @param packet Pointer to the received packet
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_receive_packet(port_id_t port_id, packet_t *packet);

/**
 * @brief Send a packet out of a port
 *
 * @param port_id Port identifier to send the packet out of
 * @param packet Pointer to the packet to send
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_send_packet(port_id_t port_id, packet_t *packet);

/**
 * @brief Send a packet with specific MAC addresses and ethertype
 *
 * This extended version allows specifying the source and destination
 * MAC addresses and the ethertype for the outgoing packet.
 *
 * @param port_id Port identifier to send the packet out of
 * @param packet Pointer to the packet to send
 * @param src_mac Source MAC address
 * @param dst_mac Destination MAC address
 * @param ethertype Ethertype value
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t port_send_packet_ext(port_id_t port_id, packet_t *packet,
                             const mac_addr_t *src_mac,
                             const mac_addr_t *dst_mac,
                             uint16_t ethertype);

// for MAC address
// Типы изменений конфигурации порта
typedef enum {
    PORT_CONFIG_MAC_CHANGED = 1,
    PORT_CONFIG_STATE_CHANGED = 2,
    PORT_CONFIG_SPEED_CHANGED = 3,
    PORT_CONFIG_MTU_CHANGED = 4
} port_config_change_t;




/**
 * @brief Получить MAC-адрес указанного порта
 * @param port_id Идентификатор порта (0-based)
 * @param mac_addr Указатель на структуру для записи MAC-адреса
 * @return STATUS_SUCCESS при успехе, код ошибки при неудаче
 * 
 * @note Функция автоматически генерирует MAC по умолчанию если порт не инициализирован
 * @note Thread-safe функция
 */
status_t port_get_mac(uint16_t port_id, mac_addr_t *mac_addr);

/**
 * @brief Установить MAC-адрес для указанного порта
 * @param port_id Идентификатор порта
 * @param mac_addr MAC-адрес для установки
 * @return STATUS_SUCCESS при успехе, код ошибки при неудаче
 * 
 * @note Валидирует MAC-адрес (не multicast, не broadcast, не нулевой)
 * @note Генерирует уведомление об изменении конфигурации
 */
status_t port_set_mac(uint16_t port_id, const mac_addr_t *mac_addr);

/**
 * @brief Получить MAC-адреса всех портов
 * @param mac_table Массив для записи MAC-адресов
 * @param table_size Размер предоставленного массива
 * @param actual_count Указатель для записи фактического количества портов
 * @return STATUS_SUCCESS при успехе
 * 
 * @note Используется для management/debug целей
 */
status_t port_get_all_macs(mac_addr_t mac_table[], uint16_t table_size, uint16_t *actual_count);






#endif /* SWITCH_SIM_PORT_H */

