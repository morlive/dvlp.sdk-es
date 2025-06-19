/**
 * @file hw_resources.h
 * @brief Hardware resources abstraction for switch simulator
 */

#ifndef SWITCH_SIM_HW_RESOURCES_H
#define SWITCH_SIM_HW_RESOURCES_H

// SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include <pthread.h>

#include "../common/types.h"
#include "../common/error_codes.h"

#include "port_types.h"  // Добавляем включение нового файла

/**
 * @brief Hardware resources types
 */
typedef enum {
    HW_RESOURCE_PORT = 0,      /**< Physical ports */
    HW_RESOURCE_BUFFER,        /**< Packet buffers */
    HW_RESOURCE_MAC_TABLE,     /**< MAC address table */
    HW_RESOURCE_VLAN_TABLE,    /**< VLAN table */
    HW_RESOURCE_ROUTE_TABLE,   /**< Routing table */
    HW_RESOURCE_ACL,           /**< Access Control Lists */
    HW_RESOURCE_COUNTER,       /**< Hardware counters */
    HW_RESOURCE_QUEUE          /**< Queue resources */
} hw_resource_type_t;

/**
 * @brief Hardware resource usage information
 */
typedef struct {
    uint32_t total;            /**< Total available resources */
    uint32_t used;             /**< Currently used resources */
    uint32_t reserved;         /**< Reserved for system use */
    uint32_t available;        /**< Available for allocation */
} hw_resource_usage_t;

/**
 * @brief Hardware context structure
 *
 * Contains information about hardware resources needed for 
 * SAI interaction with simulated hardware.
 */
typedef struct hw_context {
    void *hw_registers;           /**< Pointer to register area (simulated) */
    uint32_t port_count;          /**< Number of ports in the device */
    bool is_initialized;          /**< Hardware initialization flag */
    void *device_handle;          /**< Device handle for I/O operations */
    void *dma_memory;             /**< Pointer to simulated DMA memory */
    uint32_t device_id;           /**< Device identifier */

    pthread_mutex_t hw_mutex;     /**< Mutex for synchronizing context access */

    /* Additional fields can be added as needed */
} hw_context_t;

/**
 * @brief Hardware capabilities information
 */
typedef struct {
    bool l2_switching;         /**< L2 switching support */
    bool l3_routing;           /**< L3 routing support */
    bool vlan_filtering;       /**< VLAN filtering support */
    bool qos;                  /**< Quality of Service support */
    bool acl;                  /**< Access Control Lists support */
    bool link_aggregation;     /**< Link aggregation support */
    bool jumbo_frames;         /**< Jumbo frames support */
    bool ipv6;                 /**< IPv6 support */
    bool multicast;            /**< Multicast support */
    bool mirroring;            /**< Port mirroring support */
    uint32_t max_ports;        /**< Maximum number of ports */
    uint32_t max_vlans;        /**< Maximum number of VLANs */
    uint32_t max_mac_entries;  /**< Maximum MAC table entries */
    uint32_t max_routes;       /**< Maximum routing table entries */
} hw_capabilities_t;

/**
 * @brief Initialize hardware resources
 *
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_init(void);

/**
 * @brief Shutdown hardware resources
 *
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_shutdown(void);


/**
 * @brief Set hardware port configuration
 *
 * @param port_id Port identifier
 * @param config Port configuration to apply
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_sim_set_port_config(port_id_t port_id, const port_config_t *config);

/**
 * @brief Get the configuration of a simulated hardware port
 *
 * @param port_id ID of the port to get configuration for
 * @param config Pointer to configuration structure to be filled with port configuration
 * @return status_t Status code indicating success or error
 */
status_t hw_sim_get_port_config(port_id_t port_id, port_config_t *config);


/**
 * @brief Get hardware resource usage
 *
 * @param resource Resource type
 * @param[out] usage Resource usage information
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_get_usage(hw_resource_type_t resource, hw_resource_usage_t *usage);

/**
 * @brief Get hardware capabilities
 *
 * @param[out] capabilities Hardware capabilities information
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_get_capabilities(hw_capabilities_t *capabilities);

/**
 * @brief Reserve hardware resources
 *
 * @param resource Resource type
 * @param amount Amount to reserve
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_reserve(hw_resource_type_t resource, uint32_t amount);

/**
 * @brief Release hardware resources
 *
 * @param resource Resource type
 * @param amount Amount to release
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_release(hw_resource_type_t resource, uint32_t amount);

/**
 * @brief Check if hardware resources are available
 *
 * @param resource Resource type
 * @param amount Amount needed
 * @param[out] available Set to true if resources are available
 * @return status_t STATUS_SUCCESS if successful
 */
status_t hw_resources_check_available(hw_resource_type_t resource, uint32_t amount, bool *available);


#endif /* SWITCH_SIM_HW_RESOURCES_H */


