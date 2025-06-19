#ifndef HAL_ETHERNET_H
#define HAL_ETHERNET_H
//SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include "hal/packet.h"  // чтобы видеть struct ethernet_header_t

/** Ethernet header size dinamically from struct */
#define ETH_HEADER_SIZE (sizeof(ethernet_header_t))

#endif // HAL_ETHERNET_H

// include/hal/ethernet.h
#ifndef HAL_ETHERNET_H
#define HAL_ETHERNET_H

#include "hal/packet.h"

typedef struct __attribute__((packed)) {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} ethernet_header_t;

// Ethernet constants
// #define ETH_HEADER_SIZE     14   /**< Ethernet header size (dst+src+type) */
#define ETH_ADDR_LEN        6    /**< Ethernet address length */
#define ETH_TYPE_LEN        2    /**< Ethernet type field length */
#define ETH_CRC_LEN         4    /**< Ethernet CRC length */
#define ETH_MIN_FRAME_SIZE  64   /**< Minimum Ethernet frame size */
#define ETH_MAX_FRAME_SIZE  1518 /**< Maximum standard Ethernet frame size */
#define VLAN_TAG_SIZE       4    /**< VLAN tag size */

// Проверка совместимости на этапе компиляции
_Static_assert(sizeof(ethernet_header_t) == ETH_HEADER_SIZE,
               "ethernet_header_t size mismatch");

#endif // HAL_ETHERNET_H
