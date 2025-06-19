/**
 * @file stats.h
 * @brief Statistics collection interface for the switch simulator
 *
 * This header defines interfaces for collecting, retrieving and managing
 * statistics for ports, queues, VLANs, and other switch entities.
 */

#ifndef STATS_H
#define STATS_H
///////SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include <stdint.h>
#include <time.h>
#include "../common/error_codes.h"
#include "../common/types.h"
#include "../hal/port.h"                // -> if port_stats.h solve problem, comment this line
#include "../common/port_stats.h"

typedef struct {
    uint64_t rx_packets;        /**< Received packets for this VLAN */
    uint64_t tx_packets;        /**< Transmitted packets for this VLAN */
    uint64_t rx_bytes;          /**< Received bytes for this VLAN */
    uint64_t tx_bytes;          /**< Transmitted bytes for this VLAN */
    time_t last_clear;          /**< Timestamp of last counter clear */
} vlan_stats_t;

typedef struct {
    uint64_t enqueued;          /**< Packets added to queue */
    uint64_t dequeued;          /**< Packets removed from queue */
    uint64_t dropped;           /**< Packets dropped due to queue full */
    uint64_t current_depth;     /**< Current queue depth */
    uint64_t max_depth;         /**< Maximum queue depth reached */
    time_t last_clear;          /**< Timestamp of last counter clear */
} queue_stats_t;

typedef struct {
    uint64_t routed_packets;    /**< Total routed packets */
    uint64_t routed_bytes;      /**< Total routed bytes */
    uint64_t routing_failures;  /**< Number of routing failures */
    uint64_t arp_requests;      /**< ARP requests sent */
    uint64_t arp_replies;       /**< ARP replies received */
    time_t last_clear;          /**< Timestamp of last counter clear */
} routing_stats_t;

typedef struct stats_context_s {
    void *private_data;         /**< Private data for statistics implementation */
} stats_context_t;

status_t stats_init(stats_context_t *ctx);
status_t stats_get_port(stats_context_t *ctx, port_id_t port_id, port_stats_t *stats);
status_t stats_get_vlan(stats_context_t *ctx, vlan_id_t vlan_id, vlan_stats_t *stats);
status_t stats_get_queue(stats_context_t *ctx, port_id_t port_id, 
                             uint8_t queue_id, queue_stats_t *stats);
status_t stats_get_routing(stats_context_t *ctx, routing_stats_t *stats);
status_t stats_clear_port(stats_context_t *ctx, port_id_t port_id);
status_t stats_clear_vlan(stats_context_t *ctx, vlan_id_t vlan_id);
status_t stats_clear_queue(stats_context_t *ctx, port_id_t port_id, uint8_t queue_id);
status_t stats_clear_routing(stats_context_t *ctx);
status_t stats_clear_all(stats_context_t *ctx);
status_t stats_enable_periodic_collection(stats_context_t *ctx, uint32_t interval_ms);
status_t stats_disable_periodic_collection(stats_context_t *ctx);

status_t stats_register_counter(const char *counter_name, uint64_t *counter_ptr);

status_t stats_register_threshold_callback(stats_context_t *ctx, 
                                              const char *stat_type,
                                              uint64_t threshold,
                                              void (*callback)(void *user_data),
                                              void *user_data);
status_t stats_cleanup(stats_context_t *ctx);

#endif /* STATS_H */
