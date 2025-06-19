
#include "l3/routing_table.h"
#include "common/logging.h"
#include "common/error_codes.h"
#include "hal/hw_resources.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_ROUTES 1024
#define ROUTE_HASH_SIZE 256
#define IPV4_ADDR_LEN 4
#define IPV6_ADDR_LEN 16

typedef struct route_entry {
    routing_entry_t info;
    struct route_entry *next;       
    struct route_entry *lpm_left;   
    struct route_entry *lpm_right;  
} route_entry_t;

typedef struct {
    route_entry_t *hash_table[ROUTE_HASH_SIZE];  
    route_entry_t *lpm_root_v4;                  
    route_entry_t *lpm_root_v6;                  
    route_entry_t *route_pool;                   
    uint16_t route_count;                        
    bool hw_sync_enabled;                        
} routing_table_t;

static routing_table_t g_routing_table;
static bool g_routing_initialized = false;

static uint32_t hash_ipv4_prefix(const ipv4_addr_t *prefix, uint8_t prefix_len);
static uint32_t hash_ipv6_prefix(const ipv6_addr_t *prefix, uint8_t prefix_len);
static route_entry_t *find_route_exact(const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type);
static void insert_to_lpm_tree(route_entry_t *entry);
static void remove_from_lpm_tree(route_entry_t *entry);
static route_entry_t *find_route_lpm(const ip_addr_t *addr, ip_addr_type_t type);
static void sync_route_to_hw(const route_entry_t *entry, hw_operation_t operation);
static route_entry_t *allocate_route_entry(void);
static void free_route_entry(route_entry_t *entry);
static bool prefix_match(const ip_addr_t *addr, const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type);
static uint8_t get_common_prefix_len(const ip_addr_t *addr1, const ip_addr_t *addr2, ip_addr_type_t type);
static bool get_bit_from_prefix(const ip_addr_t *prefix, uint8_t bit_pos, ip_addr_type_t type);


routing_table_t *routing_table_get_instance(void) {
status_t routing_table_init(void) {
status_t routing_table_deinit(void) {
status_t routing_table_add_route(const routing_entry_t *route) {
status_t routing_table_delete_route(const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type) {
status_t routing_table_lookup(routing_table_t *routing_table, const ip_addr_t *dest_ip, 
                             ip_addr_type_t type, routing_entry_t *route_info) {
status_t routing_table_get_all_routes(routing_entry_t *routes, uint16_t max_routes, uint16_t *num_routes) {
status_t routing_table_set_hw_sync(bool enable) {
status_t routing_table_get_stats(routing_table_stats_t *stats) {
status_t routing_table_flush(void) {
static uint32_t hash_ipv4_prefix(const ipv4_addr_t *prefix, uint8_t prefix_len) {
static uint32_t hash_ipv6_prefix(const ipv6_addr_t *prefix, uint8_t prefix_len) {
static route_entry_t *find_route_exact(const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type) {
static void insert_to_lpm_tree(route_entry_t *entry) {
static void remove_from_lpm_tree(route_entry_t *entry) {
static route_entry_t *find_route_lpm(const ip_addr_t *addr, ip_addr_type_t type) {
static void sync_route_to_hw(const route_entry_t *entry, hw_operation_t operation) {
static route_entry_t *allocate_route_entry(void) {
static void free_route_entry(route_entry_t *entry) {
static bool prefix_match(const ip_addr_t *addr, const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type) {
static uint8_t get_common_prefix_len(const ip_addr_t *addr1, const ip_addr_t *addr2, ip_addr_type_t type) {
static bool get_bit_from_prefix(const ip_addr_t *prefix, uint8_t bit_pos, ip_addr_type_t type) {
status_t routing_add_route(const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type,
                         const ip_addr_t *next_hop, uint16_t interface_index,
                         uint16_t metric, route_source_t route_source) {
status_t routing_remove_route(const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type) {
status_t routing_lookup(const ip_addr_t *dest_addr, ip_addr_type_t type, routing_entry_t *route_info) {
static uint32_t hash_ipv4_prefix(const ipv4_addr_t *prefix, uint8_t prefix_len) {
static uint32_t hash_ipv6_prefix(const ipv6_addr_t *prefix, uint8_t prefix_len) {
static route_entry_t *find_route_exact(const ip_addr_t *prefix, uint8_t prefix_len, ip_addr_type_t type) {
static void insert_to_lpm_tree(route_entry_t *entry) {
static void remove_from_lpm_tree(route_entry_t *entry) {
static route_entry_t *find_route_lpm(const ip_addr_t *addr, ip_addr_type_t type) {
static void sync_route_to_hw(const route_entry_t *entry, hw_operation_t operation) {
static route_entry_t *allocate_route_entry(void) {
static void free_route_entry(route_entry_t *entry) {
status_t routing_table_cleanup(void) {
status_t routing_set_hw_sync(bool enable) {
status_t routing_get_stats(routing_table_stats_t *stats) {
