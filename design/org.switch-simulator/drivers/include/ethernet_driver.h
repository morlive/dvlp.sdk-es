/**
 * @file ethernet_driver.h
 * @brief Ethernet driver interface for switch simulator
 *
 * This module provides hardware abstraction layer for Ethernet interfaces
 * used in the switch simulator. It handles initialization, configuration,
 * statistics collection and packet transmission/reception operations.
 *
 * @author Your Name
 * @date May 2025
 */

#ifndef ETHERNET_DRIVER_H
#define ETHERNET_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "driver.h"
#include "common/types.h"
#include "common/error_codes.h"
#include "hal/port_types.h"
#include "hal/packet.h"

/**
 * @brief Maximum number of Ethernet ports supported by the driver
 */
#define ETH_MAX_PORTS                  64

/**
 * @brief Maximum frame size supported (including headers)
 */
#define ETH_MAX_FRAME_SIZE             9600  /* Jumbo frame support */

/**
 * @brief Minimum frame size (including headers, excluding FCS)
 */
#define ETH_MIN_FRAME_SIZE             60

/**
 * @brief Ethernet driver operational modes
 */
typedef enum {
    ETH_MODE_AUTO_NEGOTIATE = 0,   /**< Auto-negotiation for speed/duplex */
    ETH_MODE_10_HALF,              /**< 10 Mbps, half duplex */
    ETH_MODE_10_FULL,              /**< 10 Mbps, full duplex */
    ETH_MODE_100_HALF,             /**< 100 Mbps, half duplex */
    ETH_MODE_100_FULL,             /**< 100 Mbps, full duplex */
    ETH_MODE_1000_HALF,            /**< 1 Gbps, half duplex */
    ETH_MODE_1000_FULL,            /**< 1 Gbps, full duplex */
    ETH_MODE_10G_FULL,             /**< 10 Gbps, full duplex */
    ETH_MODE_25G_FULL,             /**< 25 Gbps, full duplex */
    ETH_MODE_40G_FULL,             /**< 40 Gbps, full duplex */
    ETH_MODE_100G_FULL,            /**< 100 Gbps, full duplex */
    ETH_MODE_MAX
} eth_port_mode_t;

/**
 * @brief Ethernet interface types
 */
typedef enum {
    ETH_INTERFACE_UNKNOWN = 0,     /**< Unknown interface type */
    ETH_INTERFACE_COPPER,          /**< Copper (RJ45) interface */
    ETH_INTERFACE_FIBER,           /**< Fiber optic interface */
    ETH_INTERFACE_BACKPLANE,       /**< Backplane interface */
    ETH_INTERFACE_MAX
} eth_interface_type_t;

/**
 * @brief Ethernet port status flags
 */
typedef enum {
    ETH_STATUS_LINK_UP         = (1 << 0),   /**< Link is up */
    ETH_STATUS_FULL_DUPLEX     = (1 << 1),   /**< Port is in full duplex mode */
    ETH_STATUS_ADMIN_UP        = (1 << 2),   /**< Port is administratively up */
    ETH_STATUS_LOOPBACK        = (1 << 3),   /**< Port is in loopback mode */
    ETH_STATUS_PAUSE_TX        = (1 << 4),   /**< TX pause frames enabled */
    ETH_STATUS_PAUSE_RX        = (1 << 5),   /**< RX pause frames enabled */
    ETH_STATUS_VLAN_FILTERING  = (1 << 6),   /**< VLAN filtering enabled */
    ETH_STATUS_AUTO_NEG_ACTIVE = (1 << 7)    /**< Auto-negotiation active */
} eth_status_flags_t;

/**
 * @brief Ethernet port configuration structure
 */
typedef struct {
    eth_port_mode_t mode;                  /**< Port mode configuration */
    eth_interface_type_t interface_type;   /**< Interface type */
    uint32_t flags;                        /**< Configuration flags */
    uint16_t mtu;                          /**< Maximum transmission unit */
    uint8_t mac_addr[6];                   /**< MAC address */
    bool flow_control_enabled;             /**< Flow control enabled flag */
    bool promiscuous_mode;                 /**< Promiscuous mode flag */
    bool loopback_mode;                    /**< Loopback mode flag */
} eth_port_config_t;

/**
 * @brief Ethernet port statistics structure
 */
typedef struct {
    uint64_t rx_packets;           /**< Total received packets */
    uint64_t tx_packets;           /**< Total transmitted packets */
    uint64_t rx_bytes;             /**< Total received bytes */
    uint64_t tx_bytes;             /**< Total transmitted bytes */
    uint64_t rx_errors;            /**< Receive errors */
    uint64_t tx_errors;            /**< Transmit errors */
    uint64_t rx_dropped;           /**< Packets dropped on receive */
    uint64_t tx_dropped;           /**< Packets dropped on transmit */
    uint64_t rx_unicast;           /**< Unicast packets received */
    uint64_t rx_multicast;         /**< Multicast packets received */
    uint64_t rx_broadcast;         /**< Broadcast packets received */
    uint64_t tx_unicast;           /**< Unicast packets transmitted */
    uint64_t tx_multicast;         /**< Multicast packets transmitted */
    uint64_t tx_broadcast;         /**< Broadcast packets transmitted */
    uint64_t rx_crc_errors;        /**< CRC errors on received packets */
    uint64_t rx_alignment_errors;  /**< Alignment errors on received packets */
    uint64_t collisions;           /**< Collision events */
    uint64_t rx_oversized;         /**< Oversized frames received */
    uint64_t rx_undersized;        /**< Undersized frames received */
    uint64_t rx_pause_frames;      /**< PAUSE frames received */
    uint64_t tx_pause_frames;      /**< PAUSE frames transmitted */
} eth_port_stats_t;

/**
 * @brief Ethernet port status structure
 */
typedef struct {
    uint32_t flags;                /**< Status flags (eth_status_flags_t) */
    uint32_t link_speed;           /**< Current link speed in Mbps */
    bool link_up;                  /**< Link status (true if up) */
    eth_port_mode_t negotiated_mode; /**< Negotiated port mode */
} eth_port_status_t;

/**
 * @brief Callback function type for packet reception
 *
 * @param port_id Port ID where packet was received
 * @param packet Pointer to received packet data
 * @param len Length of received packet
 * @param user_data User-defined data passed when registering callback
 *
 * @return 0 on success, error code otherwise
 */
typedef int (*eth_rx_callback_t)(uint16_t port_id, const packet_t *packet, void *user_data);

/**
 * @brief Initialize Ethernet driver subsystem
 *
 * Initializes the Ethernet driver module. Must be called before any other
 * function in this module.
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_driver_init(void);

/**
 * @brief Shut down Ethernet driver subsystem
 *
 * Cleans up resources used by the Ethernet driver module.
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_driver_shutdown(void);

/**
 * @brief Open an Ethernet port for use
 *
 * @param port_id Port identifier to open (0-based)
 * @param config Initial port configuration
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_open(uint16_t port_id, const eth_port_config_t *config);

/**
 * @brief Close an Ethernet port
 *
 * @param port_id Port identifier to close
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_close(uint16_t port_id);

/**
 * @brief Configure an Ethernet port
 *
 * @param port_id Port identifier to configure
 * @param config Port configuration
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_configure(uint16_t port_id, const eth_port_config_t *config);

/**
 * @brief Get Ethernet port configuration
 *
 * @param port_id Port identifier
 * @param[out] config Pointer to store port configuration
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_get_config(uint16_t port_id, eth_port_config_t *config);

/**
 * @brief Set port administrative state
 *
 * @param port_id Port identifier
 * @param admin_up True to set port admin up, false for admin down
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_admin_state(uint16_t port_id, bool admin_up);

/**
 * @brief Get port status
 *
 * @param port_id Port identifier
 * @param[out] status Pointer to store port status
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_get_status(uint16_t port_id, eth_port_status_t *status);

/**
 * @brief Get port statistics
 *
 * @param port_id Port identifier
 * @param[out] stats Pointer to store port statistics
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_get_stats(uint16_t port_id, eth_port_stats_t *stats);

/**
 * @brief Clear port statistics
 *
 * @param port_id Port identifier
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_clear_stats(uint16_t port_id);

/**
 * @brief Register packet receive callback for a port
 *
 * @param port_id Port identifier
 * @param callback Callback function to be called when a packet is received
 * @param user_data User-defined data to be passed to the callback
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_register_rx_callback(uint16_t port_id, eth_rx_callback_t callback, void *user_data);

/**
 * @brief Unregister packet receive callback for a port
 *
 * @param port_id Port identifier
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_unregister_rx_callback(uint16_t port_id);

/**
 * @brief Transmit a packet on a port
 *
 * @param port_id Port identifier
 * @param packet Packet to transmit
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_tx_packet(uint16_t port_id, const packet_t *packet);

/**
 * @brief Set MAC address filtering for a port
 *
 * @param port_id Port identifier
 * @param mac_addr MAC address
 * @param add true to add address to filter, false to remove
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_mac_filter(uint16_t port_id, const uint8_t mac_addr[6], bool add);

/**
 * @brief Set VLAN filtering for a port
 *
 * @param port_id Port identifier
 * @param vlan_id VLAN ID to filter
 * @param add true to add VLAN to filter, false to remove
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_vlan_filter(uint16_t port_id, uint16_t vlan_id, bool add);

/**
 * @brief Set port VLAN tagging mode
 *
 * @param port_id Port identifier
 * @param vlan_id Default VLAN ID for the port
 * @param tag_all true to tag all outgoing frames, false for selective tagging
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_vlan_tagging(uint16_t port_id, uint16_t vlan_id, bool tag_all);

/**
 * @brief Set port loopback mode
 *
 * @param port_id Port identifier
 * @param enable true to enable loopback, false to disable
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_loopback(uint16_t port_id, bool enable);

/**
 * @brief Check if a port is valid and open
 *
 * @param port_id Port identifier to check
 *
 * @return true if port is valid and open, false otherwise
 */
bool eth_port_is_valid(uint16_t port_id);

/**
 * @brief Set port flow control mode
 *
 * @param port_id Port identifier
 * @param tx_enable Enable transmission of pause frames
 * @param rx_enable Enable processing of received pause frames
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_flow_control(uint16_t port_id, bool tx_enable, bool rx_enable);

/**
 * @brief Generate a link up/down event (for simulation purposes)
 *
 * @param port_id Port identifier
 * @param link_up true for link up event, false for link down
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_simulate_link_event(uint16_t port_id, bool link_up);

/**
 * Фабрика создания Ethernet-драйвера
 * (реализована в ethernet_driver.c).
 */
/**
 * @brief Create and initialize an Ethernet driver instance
 *
 * Allocates and sets up a new Ethernet driver descriptor for the given port.
 * The returned handle must be freed by calling the driver’s shutdown operation.
 *
 * @param port_id  Port identifier to bind the driver to (0..ETH_MAX_PORTS-1)
 * @return driver_handle_t
 *         - Valid opaque handle on success
 *         - NULL on failure (e.g., out of memory or invalid port)
 */
driver_handle_t ethernet_driver_create(uint16_t port_id);


#endif /* ETHERNET_DRIVER_H */

