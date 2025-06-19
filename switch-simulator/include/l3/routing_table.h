/**
 * @file routing_table.h
 * @brief Routing table data structures and functions for IPv4 and IPv6
 */

#ifndef SDK_ES_SWITCH_SIMULATOR_INCLUDE_L3_ROUTING_TABLE_H
#define SDK_ES_SWITCH_SIMULATOR_INCLUDE_L3_ROUTING_TABLE_H


#include "../common/types.h"
#include "../common/error_codes.h"
#include "../hal/port.h"
#include "../l3/ip.h"

#define MAX_ROUTES 1024
#define MAX_INTERFACE_NAME_LEN 32

/** Route types */
typedef enum {
    ROUTE_TYPE_STATIC,
    ROUTE_TYPE_CONNECTED,
    ROUTE_TYPE_RIP,
    ROUTE_TYPE_OSPF,
    ROUTE_TYPE_BGP
} route_type_t;

/** Administrative distance values */
typedef enum {
    ADMIN_DISTANCE_CONNECTED = 0,
    ADMIN_DISTANCE_STATIC = 1,
    ADMIN_DISTANCE_OSPF = 110,
    ADMIN_DISTANCE_RIP = 120,
    ADMIN_DISTANCE_BGP_EXTERNAL = 20,
    ADMIN_DISTANCE_BGP_INTERNAL = 200
} admin_distance_t;

/** IPv4 route entry */
typedef struct {
    ipv4_addr_t destination;
    ipv4_addr_t netmask;
    ipv4_addr_t gateway;
} route_ipv4_t;

/** IPv6 route entry */
typedef struct {
    ipv6_addr_t destination;
    uint8_t prefix_len;
    ipv6_addr_t next_hop;
} route_ipv6_t;

/** Unified route entry */
typedef struct {
    bool is_ipv6;                        /**< Whether this route is IPv6 */

    union {
        route_ipv4_t ipv4;
        route_ipv6_t ipv6;
    } route;

    /* ДОБАВЛЕНИЕ: next_hop union для совместимости с ip_processing.c */
    union {
        ipv4_addr_t ipv4;                /**< IPv4 next hop (alias to route.ipv4.gateway) */
        ipv6_addr_t ipv6;                /**< IPv6 next hop (alias to route.ipv6.next_hop) */
    } next_hop;

    uint16_t interface_index;            /**< Interface index */
    port_id_t egress_port;               /**< Egress port ID */
    char interface_name[MAX_INTERFACE_NAME_LEN]; /**< Interface name */

    route_type_t type;                   /**< Route type */
    uint8_t admin_distance;              /**< Administrative distance */
    uint16_t metric;                     /**< Route metric */

    bool active;                         /**< Whether the route is active */
    bool is_connected;                   /**< Whether this is a connected route */

    uint32_t timestamp;                  /**< Last update time */
    uint32_t age;                        /**< Age in seconds */
} route_entry_t;

/** Routing table */
typedef struct {
    route_entry_t routes[MAX_ROUTES];
    uint32_t route_count;
    uint32_t last_update_time;
    bool changed;
} routing_table_t;

/* Helper macros for accessing next hop */
#define ROUTE_GET_IPV4_NEXT_HOP(route) \
    ((route)->is_ipv6 ? NULL : &(route)->route.ipv4.gateway)

#define ROUTE_GET_IPV6_NEXT_HOP(route) \
    ((route)->is_ipv6 ? &(route)->route.ipv6.next_hop : NULL)

/* API declarations */
routing_table_t *routing_table_get_instance(void);
status_t routing_table_init(routing_table_t *table);
status_t routing_table_cleanup(void);
status_t routing_table_add_route(routing_table_t *table, const route_entry_t *route);
status_t routing_table_remove_route(routing_table_t *table, ipv4_addr_t destination, ipv4_addr_t netmask);
status_t routing_table_lookup(routing_table_t *table, const ip_addr_t *dest_ip, ip_addr_type_t type, route_entry_t *route_info);
status_t routing_table_update_route(routing_table_t *table, const route_entry_t *route);
status_t routing_table_clear_routes_by_type(routing_table_t *table, route_type_t type);
uint32_t routing_table_get_count(const routing_table_t *table);
status_t routing_table_get_routes_by_type(const routing_table_t *table, route_type_t type, route_entry_t *routes, uint32_t max_routes, uint32_t *actual_routes);
status_t routing_table_get_all_routes(const routing_table_t *table, route_entry_t *routes, uint32_t max_routes, uint32_t *actual_routes);
status_t routing_table_clear(routing_table_t *table);

/* Helper for static route creation (IPv4) */
status_t routing_table_create_static_route(ipv4_addr_t destination, ipv4_addr_t netmask,
                                           ipv4_addr_t gateway, uint16_t interface_index,
                                           const char *interface_name, uint16_t metric,
                                           route_entry_t *route);

/* Helper for static route creation (IPv6) */
status_t routing_table_create_static_route_ipv6(ipv6_addr_t destination, uint8_t prefix_len,
                                               ipv6_addr_t next_hop, uint16_t interface_index,
                                               const char *interface_name, uint16_t metric,
                                               route_entry_t *route);

/* Utility functions */
ipv4_addr_t routing_table_calculate_network(ipv4_addr_t ip, ipv4_addr_t netmask);
uint8_t routing_table_get_prefix_length(ipv4_addr_t netmask);
ipv4_addr_t routing_table_create_netmask(uint8_t prefix_length);

/* Route entry initialization helpers */
status_t route_entry_init_ipv4(route_entry_t *route, ipv4_addr_t dest, ipv4_addr_t mask, ipv4_addr_t gw);
status_t route_entry_init_ipv6(route_entry_t *route, ipv6_addr_t dest, uint8_t prefix, ipv6_addr_t nh);

#endif /* SDK_ES_SWITCH_SIMULATOR_INCLUDE_L3_ROUTING_TABLE_H */
