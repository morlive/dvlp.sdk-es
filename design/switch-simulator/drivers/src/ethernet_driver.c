/**
 * @file ethernet_driver.c
 * @brief Implementation of Ethernet driver interface for switch simulator
 *
 * This module provides hardware abstraction layer for Ethernet interfaces
 * used in the switch simulator. It handles initialization, configuration,
 * statistics collection and packet transmission/reception operations.
 *
 * @author Your Name
 * @date May 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "drivers/include/ethernet_driver.h"
#include "common/logging.h"
#include "common/utils.h"
#include "drivers/include/sim_driver.h"
#include "hal/hw_resources.h"
#include "common/threading.h"

/**
 * @brief Internal port state structure
 */
typedef struct {
    bool is_open;                     /**< Port is open and initialized */
    eth_port_config_t config;         /**< Current port configuration */
    eth_port_status_t status;         /**< Current port status */
    eth_port_stats_t stats;           /**< Port statistics */
    eth_rx_callback_t rx_callback;    /**< RX callback function */
    void *rx_user_data;               /**< User data for RX callback */
    pthread_mutex_t lock;             /**< Port state lock */
} eth_port_state_t;

/**
 * @brief Global Ethernet driver state
 */
typedef struct {
    bool initialized;                       /**< Driver initialization state */
    eth_port_state_t ports[ETH_MAX_PORTS];  /**< Port state array */
    pthread_mutex_t global_lock;            /**< Global driver lock */
} eth_driver_state_t;

/* Global driver state */
static eth_driver_state_t g_eth_driver = {0};

/* Forward declarations of internal functions */
static void eth_port_init_state(eth_port_state_t *port);
static bool eth_validate_config(const eth_port_config_t *config);
static void eth_update_link_speed(uint16_t port_id);
static status_t eth_handle_received_packet(uint16_t port_id, const packet_t *packet);
static void eth_simulate_packet_processing(const packet_t *packet, eth_port_stats_t *stats);

/**
 * @brief Initialize Ethernet driver subsystem
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_driver_init(void) {
    if (g_eth_driver.initialized) {
        LOG_WARN("Ethernet driver already initialized");
        return STATUS_ALREADY_INITIALIZED;
    }

    LOG_INFO("Initializing Ethernet driver");
    
    /* Initialize the global lock */
    if (pthread_mutex_init(&g_eth_driver.global_lock, NULL) != 0) {
        LOG_ERROR("Failed to initialize Ethernet driver global lock");
        return STATUS_RESOURCE_ERROR;
    }

    /* Initialize port structures */
    for (int i = 0; i < ETH_MAX_PORTS; i++) {
        eth_port_init_state(&g_eth_driver.ports[i]);
    }

    /* Initialize hardware simulation backend */
    status_t status = sim_driver_init();
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to initialize simulation driver: %d", status);
        pthread_mutex_destroy(&g_eth_driver.global_lock);
        return status;
    }

    g_eth_driver.initialized = true;
    LOG_INFO("Ethernet driver initialized successfully");
    return STATUS_SUCCESS;
}

/**
 * @brief Shut down Ethernet driver subsystem
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_driver_shutdown(void) {
    if (!g_eth_driver.initialized) {
        LOG_WARN("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_INFO("Shutting down Ethernet driver");

    pthread_mutex_lock(&g_eth_driver.global_lock);

    /* Close all open ports */
    for (int i = 0; i < ETH_MAX_PORTS; i++) {
        if (g_eth_driver.ports[i].is_open) {
            eth_port_close(i);
        }
        pthread_mutex_destroy(&g_eth_driver.ports[i].lock);
    }

    /* Shutdown hardware simulation backend */
    sim_driver_shutdown();

    g_eth_driver.initialized = false;
    pthread_mutex_unlock(&g_eth_driver.global_lock);
    pthread_mutex_destroy(&g_eth_driver.global_lock);

    LOG_INFO("Ethernet driver shutdown complete");
    return STATUS_SUCCESS;
}

/**
 * @brief Open an Ethernet port for use
 *
 * @param port_id Port identifier to open (0-based)
 * @param config Initial port configuration
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_open(uint16_t port_id, const eth_port_config_t *config) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate configuration */
    if (config == NULL) {
        LOG_ERROR("NULL port configuration");
        return STATUS_INVALID_PARAMETER;
    }

    if (!eth_validate_config(config)) {
        LOG_ERROR("Invalid port configuration for port %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is already open */
    if (port->is_open) {
        LOG_ERROR("Port %u is already open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_ALREADY_EXISTS;
    }

    /* Initialize port state */
    memset(&port->stats, 0, sizeof(eth_port_stats_t));
    memcpy(&port->config, config, sizeof(eth_port_config_t));
    
    /* Initialize status */
    port->status.flags = ETH_STATUS_ADMIN_UP;
    if (config->flow_control_enabled) {
        port->status.flags |= (ETH_STATUS_PAUSE_TX | ETH_STATUS_PAUSE_RX);
    }
    if (config->loopback_mode) {
        port->status.flags |= ETH_STATUS_LOOPBACK;
    }
    
    /* Configure hardware simulation for this port */
    status_t status = sim_driver_port_init(port_id, config->mac_addr, config->interface_type);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to initialize simulation for port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }

    /* Simulate link negotiation */
    eth_update_link_speed(port_id);
    
    /* Mark port as open */
    port->is_open = true;
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Port %u opened successfully", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Close an Ethernet port
 *
 * @param port_id Port identifier to close
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_close(uint16_t port_id) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Shutdown simulation for this port */
    status_t status = sim_driver_port_shutdown(port_id);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to shutdown simulation for port %u: %d", port_id, status);
        /* Continue with cleanup anyway */
    }

    /* Clear port state */
    port->is_open = false;
    port->rx_callback = NULL;
    port->rx_user_data = NULL;
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Port %u closed successfully", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Configure an Ethernet port
 *
 * @param port_id Port identifier to configure
 * @param config Port configuration
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_configure(uint16_t port_id, const eth_port_config_t *config) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate configuration */
    if (config == NULL) {
        LOG_ERROR("NULL port configuration");
        return STATUS_INVALID_PARAMETER;
    }

    if (!eth_validate_config(config)) {
        LOG_ERROR("Invalid port configuration for port %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Save old configuration */
    eth_port_config_t old_config;
    memcpy(&old_config, &port->config, sizeof(eth_port_config_t));

    /* Apply new configuration */
    memcpy(&port->config, config, sizeof(eth_port_config_t));

    /* Update flow control status */
    if (config->flow_control_enabled) {
        port->status.flags |= (ETH_STATUS_PAUSE_TX | ETH_STATUS_PAUSE_RX);
    } else {
        port->status.flags &= ~(ETH_STATUS_PAUSE_TX | ETH_STATUS_PAUSE_RX);
    }

    /* Update loopback status */
    if (config->loopback_mode) {
        port->status.flags |= ETH_STATUS_LOOPBACK;
    } else {
        port->status.flags &= ~ETH_STATUS_LOOPBACK;
    }

    /* Configure hardware simulation for this port if interface type changed */
    if (old_config.interface_type != config->interface_type) {
        status_t status = sim_driver_port_update_interface(port_id, config->interface_type);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to update interface type for port %u: %d", port_id, status);
            /* Restore old configuration */
            memcpy(&port->config, &old_config, sizeof(eth_port_config_t));
            pthread_mutex_unlock(&port->lock);
            return status;
        }
    }

    /* Update MAC address if changed */
    if (memcmp(old_config.mac_addr, config->mac_addr, 6) != 0) {
        status_t status = sim_driver_port_set_mac(port_id, config->mac_addr);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR("Failed to update MAC address for port %u: %d", port_id, status);
            /* Restore old configuration */
            memcpy(&port->config, &old_config, sizeof(eth_port_config_t));
            pthread_mutex_unlock(&port->lock);
            return status;
        }
    }

    /* Re-negotiate link speed if port mode changed */
    if (old_config.mode != config->mode) {
        eth_update_link_speed(port_id);
    }

    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Port %u configured successfully", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get Ethernet port configuration
 *
 * @param port_id Port identifier
 * @param[out] config Pointer to store port configuration
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_get_config(uint16_t port_id, eth_port_config_t *config) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate output pointer */
    if (config == NULL) {
        LOG_ERROR("NULL configuration output pointer");
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Copy configuration */
    memcpy(config, &port->config, sizeof(eth_port_config_t));
    
    pthread_mutex_unlock(&port->lock);
    return STATUS_SUCCESS;
}

/**
 * @brief Set port administrative state
 *
 * @param port_id Port identifier
 * @param admin_up True to set port admin up, false for admin down
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_admin_state(uint16_t port_id, bool admin_up) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Update admin state */
    if (admin_up) {
        port->status.flags |= ETH_STATUS_ADMIN_UP;
        
        /* If admin is up, start link negotiation */
        eth_update_link_speed(port_id);
    } else {
        port->status.flags &= ~ETH_STATUS_ADMIN_UP;
        port->status.flags &= ~ETH_STATUS_LINK_UP;
        port->status.link_up = false;
    }

    /* Update simulation driver */
    sim_driver_port_set_admin_state(port_id, admin_up);
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Port %u admin state set to %s", port_id, admin_up ? "UP" : "DOWN");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get port status
 *
 * @param port_id Port identifier
 * @param[out] status Pointer to store port status
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_get_status(uint16_t port_id, eth_port_status_t *status) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate output pointer */
    if (status == NULL) {
        LOG_ERROR("NULL status output pointer");
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Copy status */
    memcpy(status, &port->status, sizeof(eth_port_status_t));
    
    pthread_mutex_unlock(&port->lock);
    return STATUS_SUCCESS;
}

/**
 * @brief Get port statistics
 *
 * @param port_id Port identifier
 * @param[out] stats Pointer to store port statistics
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_get_stats(uint16_t port_id, eth_port_stats_t *stats) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate output pointer */
    if (stats == NULL) {
        LOG_ERROR("NULL stats output pointer");
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Copy statistics */
    memcpy(stats, &port->stats, sizeof(eth_port_stats_t));
    
    pthread_mutex_unlock(&port->lock);
    return STATUS_SUCCESS;
}

/**
 * @brief Clear port statistics
 *
 * @param port_id Port identifier
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_clear_stats(uint16_t port_id) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Clear statistics */
    memset(&port->stats, 0, sizeof(eth_port_stats_t));
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Statistics cleared for port %u", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register packet receive callback for a port
 *
 * @param port_id Port identifier
 * @param callback Callback function to be called when a packet is received
 * @param user_data User-defined data to be passed to the callback
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_register_rx_callback(uint16_t port_id, eth_rx_callback_t callback, void *user_data) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate callback */
    if (callback == NULL) {
        LOG_ERROR("NULL callback function");
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Register callback */
    port->rx_callback = callback;
    port->rx_user_data = user_data;
    
    /* Register with simulation driver */
    status_t status = sim_driver_register_rx_handler(port_id, eth_handle_received_packet);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to register RX handler with simulation driver: %d", status);
        port->rx_callback = NULL;
        port->rx_user_data = NULL;
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("RX callback registered for port %u", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Unregister packet receive callback for a port
 *
 * @param port_id Port identifier
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_unregister_rx_callback(uint16_t port_id) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Unregister callback */
    port->rx_callback = NULL;
    port->rx_user_data = NULL;
    
    /* Unregister with simulation driver */
    status_t status = sim_driver_unregister_rx_handler(port_id);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to unregister RX handler with simulation driver: %d", status);
        /* Continue anyway */
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("RX callback unregistered for port %u", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Transmit a packet on a port
 *
 * @param port_id Port identifier
 * @param packet Packet to transmit
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_tx_packet(uint16_t port_id, const packet_t *packet) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate packet */
    if (packet == NULL || packet->data == NULL) {
        LOG_ERROR("NULL packet or packet data");
        return STATUS_INVALID_PARAMETER;
    }

    if (packet->length < ETH_MIN_FRAME_SIZE || packet->length > ETH_MAX_FRAME_SIZE) {
        LOG_ERROR("Invalid packet length: %u", packet->length);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Check if port is up and link is up */
    if (!(port->status.flags & ETH_STATUS_ADMIN_UP) || !port->status.link_up) {
        LOG_ERROR("Port %u is down or link is down", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_READY;
    }

    /* Update TX statistics */
    port->stats.tx_packets++;
    port->stats.tx_bytes += packet->length;
    
    /* Check packet type for more detailed statistics */
    const uint8_t *dst_mac = packet->data;
    if (dst_mac[0] & 0x01) {
        /* Multicast/broadcast packet */
        if (dst_mac[0] == 0xFF && dst_mac[1] == 0xFF && dst_mac[2] == 0xFF &&
            dst_mac[3] == 0xFF && dst_mac[4] == 0xFF && dst_mac[5] == 0xFF) {
            port->stats.tx_broadcast++;
        } else {
            port->stats.tx_multicast++;
        }
    } else {
        /* Unicast packet */
        port->stats.tx_unicast++;
    }

    /* Simulate packet processing */
    eth_simulate_packet_processing(packet, &port->stats);

    /* Forward to simulation driver */
    status_t status = sim_driver_tx_packet(port_id, packet);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to transmit packet on port %u: %d", port_id, status);
        port->stats.tx_errors++;
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    
    /* Handle loopback if enabled */
    if (port->status.flags & ETH_STATUS_LOOPBACK) {
        eth_handle_received_packet(port_id, packet);
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set MAC address filtering for a port
 *
 * @param port_id Port identifier
 * @param mac_addr MAC address
 * @param add true to add address to filter, false to remove
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_mac_filter(uint16_t port_id, const uint8_t mac_addr[6], bool add) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate MAC address */
    if (mac_addr == NULL) {
        LOG_ERROR("NULL MAC address");
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Forward to simulation driver */
    status_t status = sim_driver_set_mac_filter(port_id, mac_addr, add);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to set MAC filter on port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("MAC filter %s for port %u: %02x:%02x:%02x:%02x:%02x:%02x",
             add ? "added" : "removed", port_id,
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set VLAN filtering for a port
 *
 * @param port_id Port identifier
 * @param vlan_id VLAN ID to filter
 * @param add true to add VLAN to filter, false to remove
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_vlan_filter(uint16_t port_id, uint16_t vlan_id, bool add) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate VLAN ID */
    if (vlan_id > 4095) {
        LOG_ERROR("Invalid VLAN ID: %u", vlan_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Update VLAN filtering flag */
    if (add) {
        port->status.flags |= ETH_STATUS_VLAN_FILTERING;
    }

    /* Forward to simulation driver */
    status_t status = sim_driver_set_vlan_filter(port_id, vlan_id, add);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to set VLAN filter on port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("VLAN filter %s for port %u: VLAN %u",
             add ? "added" : "removed", port_id, vlan_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set port VLAN tagging mode
 *
 * @param port_id Port identifier
 * @param vlan_id Default VLAN ID for the port
 * @param tag_all true to tag all outgoing frames, false for selective tagging
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_vlan_tagging(uint16_t port_id, uint16_t vlan_id, bool tag_all) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate VLAN ID */
    if (vlan_id > 4095) {
        LOG_ERROR("Invalid VLAN ID: %u", vlan_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Forward to simulation driver */
    status_t status = sim_driver_set_vlan_tagging(port_id, vlan_id, tag_all);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to set VLAN tagging on port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("VLAN tagging mode set for port %u: VLAN %u, tag_all=%s",
             port_id, vlan_id, tag_all ? "true" : "false");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set port loopback mode
 *
 * @param port_id Port identifier
 * @param enable true to enable loopback, false to disable
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_loopback(uint16_t port_id, bool enable) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Update loopback flag */
    if (enable) {
        port->status.flags |= ETH_STATUS_LOOPBACK;
        port->config.loopback_mode = true;
    } else {
        port->status.flags &= ~ETH_STATUS_LOOPBACK;
        port->config.loopback_mode = false;
    }

    /* Forward to simulation driver */
    status_t status = sim_driver_set_loopback(port_id, enable);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to set loopback mode on port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Loopback mode %s for port %u", enable ? "enabled" : "disabled", port_id);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Check if a port is valid and open
 *
 * @param port_id Port identifier to check
 *
 * @return true if port is valid and open, false otherwise
 */
bool eth_port_is_valid(uint16_t port_id) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        return false;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        return false;
    }

    /* Check if port is open */
    return g_eth_driver.ports[port_id].is_open;
}

/**
 * @brief Set port flow control mode
 *
 * @param port_id Port identifier
 * @param tx_enable Enable transmission of pause frames
 * @param rx_enable Enable processing of received pause frames
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_set_flow_control(uint16_t port_id, bool tx_enable, bool rx_enable) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Update flow control flags */
    if (tx_enable) {
        port->status.flags |= ETH_STATUS_PAUSE_TX;
    } else {
        port->status.flags &= ~ETH_STATUS_PAUSE_TX;
    }

    if (rx_enable) {
        port->status.flags |= ETH_STATUS_PAUSE_RX;
    } else {
        port->status.flags &= ~ETH_STATUS_PAUSE_RX;
    }

    port->config.flow_control_enabled = (tx_enable || rx_enable);

    /* Forward to simulation driver */
    status_t status = sim_driver_set_flow_control(port_id, tx_enable, rx_enable);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to set flow control on port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Flow control set for port %u: TX=%s, RX=%s",
             port_id, tx_enable ? "enabled" : "disabled", rx_enable ? "enabled" : "disabled");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Generate a link up/down event (for simulation purposes)
 *
 * @param port_id Port identifier
 * @param link_up true for link up event, false for link down
 *
 * @return STATUS_SUCCESS on success, error code otherwise
 */
status_t eth_port_simulate_link_event(uint16_t port_id, bool link_up) {
    /* Check if driver is initialized */
    if (!g_eth_driver.initialized) {
        LOG_ERROR("Ethernet driver not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Validate port ID */
    if (port_id >= ETH_MAX_PORTS) {
        LOG_ERROR("Invalid port ID: %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    /* Lock the port state */
    eth_port_state_t *port = &g_eth_driver.ports[port_id];
    pthread_mutex_lock(&port->lock);

    /* Check if port is open */
    if (!port->is_open) {
        LOG_ERROR("Port %u is not open", port_id);
        pthread_mutex_unlock(&port->lock);
        return STATUS_NOT_FOUND;
    }

    /* Update link status */
    if (link_up) {
        port->status.flags |= ETH_STATUS_LINK_UP;
        port->status.link_up = true;
        
        /* When link comes up, negotiate speed */
        eth_update_link_speed(port_id);
    } else {
        port->status.flags &= ~ETH_STATUS_LINK_UP;
        port->status.link_up = false;
        port->status.link_speed = 0;
    }

    /* Forward to simulation driver */
    status_t status = sim_driver_simulate_link_event(port_id, link_up);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR("Failed to simulate link event on port %u: %d", port_id, status);
        pthread_mutex_unlock(&port->lock);
        return status;
    }
    
    pthread_mutex_unlock(&port->lock);
    LOG_INFO("Link %s event simulated for port %u", link_up ? "UP" : "DOWN", port_id);
    
    return STATUS_SUCCESS;
}
