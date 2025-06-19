/**
 * @file mac_learning.h
 * @brief MAC address learning interface
 */
#ifndef MAC_LEARNING_H
#define MAC_LEARNING_H

#include "../common/types.h"
#include "../common/error_codes.h"
#include "../hal/packet.h"
#include "../hal/port.h"
#include "../hal/hw_resources.h"


/**
 * @brief MAC learning statistics structure
 *
 * This structure contains statistics related to the MAC address learning process,
 * including counters for various events and current state information.
 */
typedef struct mac_learning_stats {
    /* Operational counters */
    uint32_t total_learned;        /**< Total number of MAC addresses learned since initialization */
    uint32_t total_moved;          /**< Total number of MAC address port moves detected */
    uint32_t total_aged_out;       /**< Total number of MAC addresses removed due to aging */

    /* Rate limiting statistics */
    uint32_t rate_limited;         /**< Number of MAC addresses not learned due to rate limiting */
    uint32_t rate_limit_events;    /**< Number of times rate limiting was triggered */

    /* Resource usage */
    uint32_t current_entries;      /**< Current number of entries in the MAC table */
    uint32_t max_entries;          /**< Maximum capacity of the MAC table */
    uint32_t table_full_drops;     /**< Packets dropped because MAC table was full */

    /* Performance statistics */
    uint32_t learning_latency_us;  /**< Average learning latency in microseconds */
    uint32_t lookup_latency_us;    /**< Average lookup latency in microseconds */

    /* Operational state */
    bool learning_enabled;         /**< Whether global MAC learning is currently enabled */
    uint32_t last_reset_time;      /**< Timestamp of the last statistics reset (seconds since boot) */
} mac_learning_stats_t;


/**
 * @brief Initialize the MAC learning module
 *
 * @param num_ports Number of ports in the system
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_learning_init(uint32_t num_ports);

/**
 * @brief Get current MAC learning statistics
 * 
 * Retrieves the current statistics of the MAC learning process.
 * 
 * @param[out] stats Pointer to structure to store statistics
 * @return status_t STATUS_SUCCESS on success, error code otherwise
 */
status_t mac_learning_get_stats(mac_learning_stats_t *stats);

/**
 * @brief Reset MAC learning statistics counters
 * 
 * Resets all counter fields in the MAC learning statistics to zero.
 * Does not modify state information like learning_enabled.
 *
 * @return status_t STATUS_SUCCESS on success, error code otherwise
 */
status_t mac_learning_reset_stats(void);

/**
 * @brief Cleanup and release resources used by the MAC learning module
 *
 * @return status_t STATUS_SUCCESS on success
 */
status_t mac_learning_cleanup(void);


// Здесь можно добавить прототипы других функций из mac_learning.c



#endif /* MAC_LEARNING_H */


