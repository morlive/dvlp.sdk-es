
///**
// * @brief Comprehensive port statistics structure
// */
//typedef struct {
//    uint64_t rx_packets;        /**< Total received packets */
//    uint64_t tx_packets;        /**< Total transmitted packets */
//    uint64_t rx_bytes;          /**< Total received bytes */
//    uint64_t tx_bytes;          /**< Total transmitted bytes */
//    uint64_t rx_errors;         /**< Receive errors */
//    uint64_t tx_errors;         /**< Transmit errors */
//    uint64_t rx_drops;          /**< Dropped received packets */
//    uint64_t tx_drops;          /**< Dropped transmit packets */
//    uint64_t rx_unicast;        /**< Received unicast packets */
//    uint64_t tx_unicast;        /**< Transmitted unicast packets */
//    uint64_t rx_broadcast;      /**< Received broadcast packets */
//    uint64_t tx_broadcast;      /**< Transmitted broadcast packets */
//    uint64_t rx_multicast;      /**< Received multicast packets */
//    uint64_t tx_multicast;      /**< Transmitted multicast packets */
//    uint64_t collisions;        /**< Collision count */
//    time_t last_clear;          /**< Timestamp of last counter clear */
//} port_stats_t;


/**
 * @file port_stats.h
 * @brief Port statistics counters and size-based packet bucket statistics.
 *
 * This structure holds comprehensive counters for Ethernet port statistics,
 * including total packets/bytes, errors, drops, unicast/broadcast/multicast counts,
 * and detailed transmit packet size buckets.
 */
#ifndef SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_PORT_STATS_H
#define SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_PORT_STATS_H

#include <stdint.h>
#include <time.h>


typedef struct port_stats_t {
    /*----------------------------------------------------------------------*/
    /* Total packet and byte counters                                       */
    /*----------------------------------------------------------------------*/
    uint64_t rx_packets; /**< Total number of packets received */
    uint64_t tx_packets; /**< Total number of packets transmitted */
    uint64_t rx_bytes;   /**< Total number of bytes received */
    uint64_t tx_bytes;   /**< Total number of bytes transmitted */

    /*----------------------------------------------------------------------*/
    /* Error and drop counters                                              */
    /*----------------------------------------------------------------------*/
    uint64_t rx_errors;  /**< Total receive errors */
    uint64_t tx_errors;  /**< Total transmit errors */
    uint64_t rx_drops;   /**< Total dropped received packets */
    uint64_t tx_drops;   /**< Total dropped transmitted packets */

    /*----------------------------------------------------------------------*/
    /* Multicast/Unicast/Broadcast packet counters                           */
    /*----------------------------------------------------------------------*/
    uint64_t rx_unicast;   /**< Received unicast packets */
    uint64_t tx_unicast;   /**< Transmitted unicast packets */
    uint64_t rx_broadcast; /**< Received broadcast packets */
    uint64_t tx_broadcast; /**< Transmitted broadcast packets */
    uint64_t rx_multicast; /**< Received multicast packets */
    uint64_t tx_multicast; /**< Transmitted multicast packets */

    /*----------------------------------------------------------------------*/
    /* Collision and clear timestamp                                         */
    /*----------------------------------------------------------------------*/
    uint64_t collisions; /**< Total number of collisions detected */
    time_t   last_clear; /**< Timestamp of last statistics reset */

    /*----------------------------------------------------------------------*/
    /* Transmit packet size bucket counters (in bytes)                       */
    /*----------------------------------------------------------------------*/
    uint64_t tx_packets_lt_64;      /**< Packets with length < 64 bytes */
    uint64_t tx_packets_64;         /**< Packets with length == 64 bytes */
    uint64_t tx_packets_65_127;     /**< Packets with length 65–127 bytes */
    uint64_t tx_packets_128_255;    /**< Packets with length 128–255 bytes */
    uint64_t tx_packets_256_511;    /**< Packets with length 256–511 bytes */
    uint64_t tx_packets_512_1023;   /**< Packets with length 512–1023 bytes */
    uint64_t tx_packets_1024_1518;  /**< Packets with length 1024–1518 bytes */
    uint64_t tx_packets_1519_max;   /**< Packets with length >= 1519 bytes */
} port_stats_t;



#endif /* SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_PORT_STATS_H */

