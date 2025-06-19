/**
 * @file ip_processing.h
 * @brief Interface for IP packet processing functionality
 *
 * This file declares the interfaces for processing IP packets in the switching/routing system.
 * It handles IPv4 and IPv6 packet processing, header validation, fragmentation,
 * time-to-live management, and forwarding decision logic.
 *
 * @copyright (c) 2025 Switch Simulator Project
 * @license Proprietary
 */

#ifndef IP_PROCESSING_H
#define IP_PROCESSING_H

#include "common/error_codes.h"
#include "common/types.h"
#include "hal/packet.h"
#include "hal/port.h"
#include "l3/ip.h"
#include "l3/routing_table.h"

/* Constants for IP processing */
#define IP_VERSION_4             4
#define IP_VERSION_6             6
#define IPV4_HEADER_MIN_LEN      20      /* Minimum IPv4 header length in bytes */
#define IPV4_HEADER_MAX_LEN      60      /* Maximum IPv4 header length in bytes */
#define IPV6_HEADER_LEN          40      /* Fixed IPv6 header length in bytes */
#define MAX_FRAGMENTS            64      /* Maximum fragments per packet */
#ifndef DEFAULT_MTU
#define DEFAULT_MTU              1500    /* Default Ethernet MTU size in bytes */
#endif

/* IP Protocol Numbers (common ones) */
#define IP_PROTO_ICMP            1
#define IP_PROTO_IGMP            2
#define IP_PROTO_TCP             6
#define IP_PROTO_UDP             17
#define IP_PROTO_IPV6            41
#define IP_PROTO_ICMPV6          58
#define IP_PROTO_OSPF            89

//typedef struct {
//    uint64_t packets_processed;
//    uint64_t bytes_processed;
//    uint64_t ipv4_packets;
//    uint64_t ipv6_packets;
//    uint64_t fragmented_packets;
//    uint64_t reassembled_packets;
//    uint64_t ttl_exceeded;
//    uint64_t header_errors;
//    uint64_t forwarded_packets;
//    uint64_t local_delivered;
//    uint64_t dropped_packets;
//} ip_stats_t;

/* For API compatibility */
//typedef ip_stats_t ip_statistics_t;
typedef packet_buffer_t packet_t;

status_t ip_processing_init(void);
status_t ip_processing_shutdown(void);
status_t ip_process_packet(packet_buffer_t *packet, uint16_t *offset);
status_t ip_process_packet_with_offset(packet_buffer_t *packet, uint16_t *offset);
status_t ip_set_port_mtu(port_id_t port_id, uint16_t mtu);
status_t ip_get_port_mtu(port_id_t port_id, uint16_t *mtu);
//status_t ip_get_statistics(ip_statistics_t *stats);
status_t ip_create_packet(const void *src_addr, const void *dst_addr, uint8_t protocol,
                         uint8_t ttl, const uint8_t *data, uint16_t data_len,
                         bool is_ipv6, packet_buffer_t **packet);

#endif /* IP_PROCESSING_H */
