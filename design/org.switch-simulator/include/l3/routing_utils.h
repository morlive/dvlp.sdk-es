#ifndef ROUTING_UTILS_H
#define ROUTING_UTILS_H

#include "routing_table.h"

static inline const ipv4_addr_t *route_get_ipv4_gateway(const route_entry_t *route) {
    return &route->route.ipv4.gateway;
}

#endif // ROUTING_UTILS_H

