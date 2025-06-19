/**
 * @file mac_table.h
 * @brief MAC address table management interface
 */
#ifndef SWITCH_SIM_MAC_TABLE_H
#define SWITCH_SIM_MAC_TABLE_H

///SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include "../common/types.h"
#include "../common/error_codes.h"
#include "../common/threading.h"
#include "../hal/packet.h"
#include "../hal/port.h"
#include "../hal/hw_resources.h"

// #define MAX_MAC_TABLE_ENTRIES 8192

typedef struct mac_table_internal mac_table_internal_t;

/**
 * @brief MAC address entry types
 */
typedef enum {
    MAC_ENTRY_TYPE_DYNAMIC = 0,  /**< Dynamically learned MAC address */
    MAC_ENTRY_TYPE_STATIC,       /**< Statically configured MAC address */
    MAC_ENTRY_TYPE_MANAGEMENT    /**< Management MAC address (e.g. CPU port) */
} mac_entry_type_t;

/**
 * @brief MAC table entry aging state
 */
typedef enum {
    MAC_AGING_ACTIVE = 0,  /**< Entry is subject to aging */
    MAC_AGING_DISABLED     /**< Entry is not subject to aging */
} mac_aging_state_t;

/**
 * @brief MAC table entry structure
 */
typedef struct {
    mac_addr_t     mac_addr;      /**< MAC address */
    vlan_id_t      vlan_id;       /**< VLAN ID */
    port_id_t      port_id;       /**< Port ID where MAC was learned */
    mac_entry_type_t type;        /**< Entry type */
    mac_aging_state_t aging;      /**< Aging state */
    uint32_t       age_timestamp;  /**< Last time this entry was used (for aging) */
} mac_table_entry_t;


/**
 * @brief Extended information about MAC table entry for iteration
 */
typedef struct {
    mac_addr_t     mac_addr;      /**< MAC address */
    vlan_id_t      vlan_id;       /**< VLAN ID */
    port_id_t      port_id;       /**< Port ID where MAC was learned */
    mac_entry_type_t type;        /**< Entry type */
    mac_aging_state_t aging;      /**< Aging state */
    uint32_t       age_timestamp;  /**< Last time this entry was used (for aging) */
    uint32_t       hit_count;      /**< Number of times this entry was matched */
    uint64_t       creation_time;  /**< Time when entry was created */
    uint64_t       last_used_time; /**< Time when entry was last used */
} mac_entry_info_t;


/**
 * @brief Callback function type for MAC address table events
 *
 * This callback is invoked when MAC address entries are added, updated, or deleted
 * in the MAC address table. It allows subscribers to react to changes in the 
 * MAC address table in real-time.
 *
 * @param entry Pointer to the MAC table entry that was changed
 * @param is_added true if the entry was added or updated, false if it was deleted
 * @param user_data User-provided context data passed during callback registration
 */
typedef void (*mac_event_callback_t)(mac_table_entry_t *entry, bool is_added, void *user_data);


/**
 * @brief Callback function type for iterating through MAC table entries
 *
 * @param entry Pointer to the current MAC table entry
 * @param user_data User data passed to the iterator function
 * @return bool true to continue iteration, false to stop
 */
typedef bool (*mac_table_iter_cb_t)(mac_table_entry_t *entry, void *user_data);


/**
 * @brief MAC learning configuration
 */
typedef struct {
    bool           learning_enabled;  /**< Global MAC learning enabled/disabled */
    uint32_t       aging_time;        /**< MAC entry aging time in seconds, 0 = no aging */
    uint32_t       max_entries;       /**< Maximum number of entries in the table */
    bool           move_detection;    /**< Enable/disable MAC move detection */
} mac_table_config_t;

/**
 * @brief MAC table structure
 */
typedef struct {
    mac_table_config_t config;            /**< MAC table configuration */
    void *hash_table;                     /**< Hash table for MAC entries lookup */
    uint32_t current_entries;             /**< Current number of entries in the table */
    uint32_t dynamic_entries;             /**< Number of dynamic entries */
    uint32_t static_entries;              /**< Number of static entries */
    uint32_t last_aging_time;             /**< Timestamp of last aging process run */
    spinlock_t lock;                      /**< Lock for thread-safe access */
    mac_event_callback_t event_callback;  /**< Callback for MAC table events */
    void *callback_user_data;             /**< User data for callback */
    bool port_learning_map[MAX_PORTS];    /**< Per-port learning configuration */
    uint32_t mac_move_count;              /**< Counter for MAC move detection */
    bool initialized;                     /**< Table initialization state */
} mac_table_t;

/**
 * @brief Structure for MAC table statistics
 */
typedef struct {
    uint32_t total_entries;     /**< Total number of entries in the table */
    uint32_t static_entries;    /**< Number of static entries */
    uint32_t dynamic_entries;   /**< Number of dynamic entries */
    uint32_t table_size;        /**< Size of the hash table */
    uint32_t aging_time;        /**< Current aging time in seconds */
} mac_table_stats_t;



/**
 * @brief Get pointer to the global MAC table instance
 * @return Pointer to the global MAC table
 */
mac_table_internal_t* mac_table_get_instance(void);

/**
 * @brief Initialize the MAC address table
 *
 * @param size Hash table size (0 for default)
 * @param aging_time Aging time in seconds (0 for default)
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_init(uint32_t size, uint32_t aging_time);

/**
 * @brief Deinitialize the MAC address table
 *
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_deinit(void);

/**
 * @brief Check if MAC table has sufficient resources for new entries
 *
 * @param count Number of entries to check
 * @param available Output parameter set to true if resources are available
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_check_resources(uint32_t count, bool *available);

/**
 * @brief Get MAC table statistics
 *
 * @param stats Pointer to statistics structure to fill
 * @return status_t Status code
 */
status_t mac_table_get_stats(mac_table_stats_t *stats);

/**
 * @brief Add a static entry to the MAC table
 *
 * @param mac_addr MAC address
 * @param vlan_id VLAN ID
 * @param port_id Port ID
 * @return status_t STATUS_SUCCESS on success, error code otherwise
 */
status_t mac_table_add_static_entry(mac_addr_t mac_addr, vlan_id_t vlan_id, port_id_t port_id);

/**
 * @brief Delete an entry from the MAC table
 *
 * @param mac_addr MAC address
 * @param vlan_id VLAN ID
 * @return status_t STATUS_SUCCESS on success, error code otherwise
 */
status_t mac_table_delete_entry(mac_addr_t mac_addr, vlan_id_t vlan_id);

/**
 * @brief Look up a MAC address in the table
 *
 * @param mac_addr MAC address to look up
 * @param vlan_id VLAN ID
 * @param entry Output parameter to store the entry if found
 * @return status_t STATUS_SUCCESS if found, STATUS_NOT_FOUND otherwise
 */
//status_t mac_table_lookup(mac_addr_t mac_addr, vlan_id_t vlan_id, mac_table_entry_t *entry);
status_t mac_table_lookup(const mac_addr_t mac, vlan_id_t vlan_id, port_id_t *port_id);

/**
 * @brief Add a MAC entry to the table (static or dynamic)
 *
 * @param mac MAC address
 * @param port_id Port ID associated with the MAC
 * @param vlan_id VLAN ID
 * @param is_static true if entry is static, false if dynamic
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_add(const mac_addr_t    mac, port_id_t port_id, vlan_id_t vlan_id, bool is_static);

/**
 * @brief Process MAC learning for an incoming packet
 *
 * @param packet Packet information
 * @param port_id Port on which packet was received
 * @return status_t STATUS_SUCCESS on success, error code otherwise
 */
status_t mac_table_learn(packet_info_t *packet, port_id_t port_id);

/**
 * @brief Flush entries in the MAC table matching specified parameters
 *
 * @param vlan_id VLAN ID to flush (0 = all VLANs)
 * @param port_id Port ID to flush (0 = all ports)
 * @param flush_static Whether to flush static entries
 * @return status_t Status of the operation
 */
status_t mac_table_flush(vlan_id_t vlan_id, port_id_t port_id, bool flush_static);

/**
 * @brief Get destination port for forwarding a packet
 *
 * @param dst_mac Destination MAC address
 * @param vlan_id VLAN ID
 * @param port_id Output parameter to store destination port
 * @return status_t STATUS_SUCCESS if found, STATUS_NOT_FOUND otherwise
 */
status_t mac_table_get_port(mac_addr_t dst_mac, vlan_id_t vlan_id, port_id_t *port_id);

/**
 * @brief Clear all dynamic entries in the MAC table
 *
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_clear_dynamic(void);

/**
 * @brief Clear all entries (both static and dynamic) in the MAC table
 *
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_clear_all(void);

/**
 * @brief Process MAC aging - remove entries that have expired
 *
 * @param table Pointer to the MAC table for processing
 * @param current_time Current time in seconds for comparing with the entries' aging time
 * @return status_t STATUS_SUCCESS on successful execution
 */
status_t mac_table_process_aging(mac_table_t *table, uint32_t current_time);

/**
 * @brief Get number of entries currently in the MAC table
 *
 * @param count Output parameter to store entry count
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_get_count(uint32_t *count);

/**
 * @brief Get MAC table resource usage
 *
 * @param usage Output parameter to store usage information
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_get_resource_usage(hw_resource_usage_t *usage);

/**
 * @brief Get all entries in the MAC table
 *
 * @param entries Array to store MAC table entries
 * @param max_entries Size of the entries array
 * @param count Output parameter to store number of entries returned
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_get_entries(mac_table_entry_t *entries, uint32_t max_entries, uint32_t *count);

/**
 * @brief Iterate through all entries in the MAC table
 *
 * @param callback Function to call for each entry
 * @param user_data User data to pass to callback
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_iterate(mac_table_iter_cb_t callback, void *user_data);

/**
 * @brief Configure MAC learning on a specific port
 *
 * @param port_id Port identifier
 * @param enable Enable/disable MAC learning
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_configure_port_learning(port_id_t port_id, bool enable);

/**
 * @brief Register callback for MAC address change events
 *
 * @param callback Function to call when MAC events occur
 * @param user_data User data to pass to callback
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_register_event_callback(mac_event_callback_t callback, void *user_data);

/**
 * @brief Unregister previously registered MAC event callback
 *
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_table_unregister_event_callback(void);


#endif /* SWITCH_SIM_MAC_TABLE_H */

