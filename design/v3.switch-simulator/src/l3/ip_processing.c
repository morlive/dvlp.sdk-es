
#define _POSIX_C_SOURCE 200809L


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "common/error_codes.h"
#include "common/logging.h"
#include "common/types.h"
#include "common/utils.h"

#include "hal/packet.h"
#include "hal/port.h"
#include "l2/vlan.h"
#include "l3/ip.h"
#include "l3/ip_processing.h"
#include "l3/routing_table.h"
#include "l3/arp.h"
#include "management/stats.h"

extern routing_table_t *routing_table_get_instance(void);

#define IP_VERSION_4             4
#define IP_VERSION_6             6
#define IPV4_HEADER_MIN_LEN      20
#define IPV4_HEADER_MAX_LEN      60
#define IPV6_HEADER_LEN          40
#define IP_FLAG_DF               0x4000
#define IP_FLAG_MF               0x2000
#define IP_FRAG_OFFSET_MASK      0x1FFF
#define IP_FRAGMENT_UNIT         8
#define TTL_DEFAULT              64
#define TTL_THRESHOLD            1
#define IPV6_HOP_LIMIT_DEFAULT   64
#define IPV6_HOP_LIMIT_THRESHOLD 1

#define IP_PROTO_ICMP            1
#define IP_PROTO_IGMP            2
#define IP_PROTO_TCP             6
#define IP_PROTO_UDP             17
#define IP_PROTO_IPV6            41
#define IP_PROTO_ICMPV6          58
#define IP_PROTO_OSPF            89

#define IPV6_EXT_HOP_BY_HOP      0
#define IPV6_EXT_ROUTING         43
#define IPV6_EXT_FRAGMENT        44
#define IPV6_EXT_ESP             50
#define IPV6_EXT_AUTH            51
#define IPV6_EXT_DEST_OPTS       60

typedef struct {
    uint64_t packets_processed;
    uint64_t bytes_processed;
    uint64_t ipv4_packets;
    uint64_t ipv6_packets;
    uint64_t fragmented_packets;
    uint64_t reassembled_packets;
    uint64_t ttl_exceeded;
    uint64_t header_errors;
    uint64_t forwarded_packets;
    uint64_t local_delivered;
    uint64_t dropped_packets;
} ip_stats_t;

static ip_stats_t g_ip_stats;

static uint16_t g_port_mtu_table[MAX_PORTS];

typedef struct ipv4_frag_entry {
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
    uint16_t ident;
    uint8_t protocol;
    uint32_t arrival_time;
    uint32_t total_length;
    uint16_t fragment_flags;
    uint8_t *reassembled_data;
    uint16_t fragments_received;
    bool fragments[MAX_FRAGMENTS];
    struct ipv4_frag_entry *next;
} ipv4_frag_entry_t;

typedef struct ipv6_frag_entry {
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
    uint32_t ident;
    uint8_t next_header;
    uint32_t arrival_time;
    uint32_t total_length;
    uint8_t *reassembled_data;
    uint16_t fragments_received;
    bool fragments[MAX_FRAGMENTS];
    struct ipv6_frag_entry *next;
} ipv6_frag_entry_t;

static ipv4_frag_entry_t *g_ipv4_frag_table = NULL;
static ipv6_frag_entry_t *g_ipv6_frag_table = NULL;

typedef struct {
    uint8_t current_header;
    uint16_t current_offset;
    uint8_t next_header;
    bool has_fragment_header;
    bool has_routing_header;
    uint8_t routing_type;
    uint8_t segments_left;
} ipv6_ext_headers_ctx_t;

static status_t process_ipv4_packet(packet_buffer_t *packet, uint16_t *offset);
static status_t process_ipv6_packet(packet_buffer_t *packet, uint16_t *offset);

status_t stats_register_counter(const char *counter_name, uint64_t *counter_ptr);

static status_t validate_ipv4_header(const ipv4_header_t *header, uint16_t packet_len);
static status_t validate_ipv6_header(const ipv6_header_t *header, uint16_t packet_len);
static status_t process_ipv4_options(const ipv4_header_t *header, packet_buffer_t *packet);
static status_t process_ipv6_extension_headers(packet_buffer_t *packet, uint16_t *offset, ipv6_ext_headers_ctx_t *ctx);
static status_t fragment_ipv4_packet(packet_buffer_t *packet, uint16_t mtu, port_id_t egress_port);
static status_t fragment_ipv6_packet(packet_buffer_t *packet, uint16_t mtu, port_id_t egress_port);
static ipv4_frag_entry_t *find_ipv4_frag_entry(const ipv4_header_t *header);
static ipv6_frag_entry_t *find_ipv6_frag_entry(const ipv6_addr_t *src, const ipv6_addr_t *dst, uint32_t ident);
static status_t reassemble_ipv4_fragments(ipv4_frag_entry_t *entry, packet_t **reassembled);
static status_t reassemble_ipv6_fragments(ipv6_frag_entry_t *entry, packet_t **reassembled);
static void cleanup_stale_fragments(void);
static status_t forward_ip_packet(packet_buffer_t *packet, const route_entry_t *route);
static status_t deliver_to_local_stack(packet_buffer_t *packet, uint8_t protocol);
static uint16_t calculate_ipv4_checksum(const void *data, size_t len);
static bool is_local_address(const void *addr, bool is_ipv6);

uint32_t get_system_time_ms(void)
status_t ip_processing_init(void) {
status_t ip_processing_shutdown(void) {
status_t ip_process_packet(packet_buffer_t *packet, uint16_t *offset) {
status_t ip_set_port_mtu(port_id_t port_id, uint16_t mtu) {
status_t ip_get_port_mtu(port_id_t port_id, uint16_t *mtu) {
status_t ip_get_statistics(ip_stats_t *stats) {
status_t ip_create_packet(const void *src_addr, const void *dst_addr, uint8_t protocol,

static status_t process_ipv4_packet(packet_buffer_t *packet, uint16_t *offset) {
static status_t process_ipv6_packet(packet_buffer_t *packet, uint16_t *offset) {
static status_t validate_ipv4_header(const ipv4_header_t *header, uint16_t packet_len) {
static status_t validate_ipv6_header(const ipv6_header_t *header, uint16_t packet_len) {
static status_t process_ipv4_options(const ipv4_header_t *header, packet_buffer_t *packet) {
static status_t process_ipv6_extension_headers(packet_buffer_t *packet, uint16_t *offset, ipv6_ext_headers_ctx_t *ctx) {
static uint16_t calculate_ipv4_checksum(const void *data, size_t len) {
static bool is_local_address(const void *addr, bool is_ipv6) {
static ipv4_frag_entry_t *find_ipv4_frag_entry(const ipv4_header_t *header) {
static ipv6_frag_entry_t *find_ipv6_frag_entry(const ipv6_addr_t *src, const ipv6_addr_t *dst, uint32_t ident) {
static void cleanup_stale_fragments(void) {
static status_t reassemble_ipv4_fragments(ipv4_frag_entry_t *entry, packet_t **reassembled) {
static status_t reassemble_ipv6_fragments(ipv6_frag_entry_t *entry, packet_t **reassembled) {
static status_t forward_ip_packet(packet_buffer_t *packet, const route_entry_t *route) {
static status_t fragment_ipv4_packet(packet_buffer_t *packet, uint16_t mtu, port_id_t egress_port) {
static status_t fragment_ipv6_packet(packet_buffer_t *packet, uint16_t mtu, port_id_t egress_port) {
static status_t deliver_to_local_stack(packet_buffer_t *packet, uint8_t protocol) {
static uint16_t calculate_ipv4_checksum(const void *data, size_t len) {
static bool is_local_address(const void *addr, bool is_ipv6) {

