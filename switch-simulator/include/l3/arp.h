/**
 * @file arp.h
 * @brief Header file for ARP (Address Resolution Protocol) functionality
 *
 * This file contains declarations for ARP table management and related operations,
 * including ARP cache maintenance, lookups, and packet processing.
 */

#ifndef SDK_ES_SWITCH_SIMULATOR_INCLUDE_L3_ARP_H
#define SDK_ES_SWITCH_SIMULATOR_INCLUDE_L3_ARP_H


#include "common/types.h"
#include "hal/packet.h"
#include <stdint.h>
#include <stdbool.h>

/* ARP specific status codes */
#define ARP_STATUS_SUCCESS            0x00010000  /**< ARP operation completed successfully */
#define ARP_STATUS_PENDING            0x00010001  /**< ARP resolution is in progress */
#define ARP_STATUS_CACHE_MISS         0x00010002  /**< Entry not found in ARP cache */
#define ARP_STATUS_TABLE_FULL         0x00010003  /**< ARP table is full */
#define ARP_STATUS_ENTRY_EXISTS       0x00010004  /**< Entry already exists in ARP table */
#define ARP_STATUS_INVALID_MAC        0x00010005  /**< Invalid MAC address format or value */
#define ARP_STATUS_INVALID_IP         0x00010006  /**< Invalid IP address format or value */
#define ARP_STATUS_TIMEOUT            0x00010007  /**< ARP request timed out */
#define ARP_STATUS_NO_ROUTE           0x00010008  /**< No route to destination IP */
#define ARP_STATUS_UNREACHABLE        0x00010009  /**< Host unreachable */
#define ARP_STATUS_INCOMPLETE         0x0001000A  /**< ARP entry exists but is incomplete */
#define ARP_STATUS_INVALID_OPERATION  0x0001000B  /**< Operation not valid for this entry state */
#define ARP_STATUS_HARDWARE_ERROR     0x0001000C  /**< Hardware error during ARP operation */
#define ARP_STATUS_QUEUE_FULL         0x0001000D  /**< ARP request queue is full */
#define ARP_STATUS_NOT_INITIALIZED    0x0001000E  /**< ARP subsystem not initialized */
#define ARP_STATUS_INVALID_INTERFACE  0x0001000F  /**< Interface not valid for ARP operation */
#define ARP_STATUS_ENTRY_STALE        0x00010010  /**< ARP entry exists but is stale */
#define ARP_STATUS_PROXY_DENIED       0x00010011  /**< Proxy ARP denied for this address */
#define ARP_STATUS_INVALID_PACKET     0x00010012  /**< Invalid ARP packet format */
#define ARP_STATUS_THROTTLED          0x00010013  /**< ARP requests being throttled */

/* ARP entry state codes for public API */
#define ARP_ENTRY_STATE_INCOMPLETE  0
#define ARP_ENTRY_STATE_REACHABLE   1
#define ARP_ENTRY_STATE_STALE       2
#define ARP_ENTRY_STATE_DELAY       3
#define ARP_ENTRY_STATE_PROBE       4
#define ARP_ENTRY_STATE_FAILED      5

/* ARP statistics structure */
typedef struct {
    uint64_t requests_sent;          /* Number of ARP requests sent */
    uint64_t requests_received;      /* Number of ARP requests received */
    uint64_t replies_sent;           /* Number of ARP replies sent */
    uint64_t replies_received;       /* Number of ARP replies received */
    uint64_t cache_hits;             /* Number of successful ARP cache lookups */
    uint64_t cache_misses;           /* Number of ARP cache lookup misses */
    uint64_t cache_flushes;          /* Number of times ARP cache was flushed */
    uint64_t cache_full_errors;      /* Number of times cache was full */
    uint64_t invalid_packets;        /* Number of invalid ARP packets received */
    uint64_t entries_added;          /* Number of entries added to ARP table */
    uint64_t entries_removed;        /* Number of entries removed from ARP table */    
    uint64_t entries_aged;           /* Number of entries removed due to aging */
    uint64_t current_entries;        /* Current number of entries in ARP table */  
} arp_stats_t;

/* ARP entry information structure - used for retrieving entries */
typedef struct {
    ipv4_addr_t ip;                 /* IPv4 address */
    mac_addr_t mac;                 /* MAC address */
    uint16_t port_index;            /* Port where this MAC was learned */
    uint32_t age;                   /* Age of the entry in seconds */
    bool is_static;                 /* Whether the entry is static */
//    arp_state_t state;              /* State of the ARP entry */
    uint8_t state;                  /* State of the ARP entry */
} arp_entry_info_t;

/* Forward declaration of ARP table structure */
typedef struct arp_table_s arp_table_t;

/**
 * @brief Get pointer to the global ARP table instance
 *
 * @return arp_table_t* Pointer to the global ARP table
 */
arp_table_t* arp_table_get_instance(void);

/**
 * @brief Initialize the ARP subsystem
 * 
 * @param table Pointer to the ARP table structure
 * @return status_t Status of the operation
 */
status_t arp_init(arp_table_t *table);

/**
 * @brief Deinitialize the ARP subsystem and free resources
 * 
 * @param table Pointer to the ARP table structure
 * @return status_t Status of the operation
 */
status_t arp_deinit(arp_table_t *table);

/**
 * @brief Add an entry to the ARP table
 * 
 * @param table Pointer to the ARP table structure
 * @param ipv4 Pointer to the IPv4 address
 * @param mac Pointer to the MAC address
 * @param port_index Port index where the MAC was learned
 * @return status_t Status of the operation
 */
status_t arp_add_entry(arp_table_t *table, const ipv4_addr_t *ipv4, const mac_addr_t *mac, uint16_t port_index);

/**
 * @brief Look up a MAC address for a given IPv4 address
 * 
 * @param table Pointer to the ARP table structure
 * @param ipv4 Pointer to the IPv4 address to look up
 * @param mac_result Pointer to store the found MAC address
 * @param port_index_result Pointer to store the port index
 * @return status_t Status of the operation
 */
status_t arp_lookup(arp_table_t *table, const ipv4_addr_t *ipv4, mac_addr_t *mac_result, uint16_t *port_index_result);

/**
 * @brief Remove an entry from the ARP table
 * 
 * @param table Pointer to the ARP table structure
 * @param ipv4 Pointer to the IPv4 address to remove
 * @return status_t Status of the operation
 */
status_t arp_remove_entry(arp_table_t *table, const ipv4_addr_t *ipv4);

/**
 * @brief Process an incoming ARP packet
 * 
 * @param table Pointer to the ARP table structure
 * @param packet Pointer to the packet structure
 * @param port_index Port on which the packet was received
 * @return status_t Status of the operation
 */
//status_t arp_process_packet(arp_table_t *table, const packet_buffer_t *packet, uint16_t port_index);
/**
 * @brief Handle an Ethernet frame that contains an ARP packet
 *
 * Демультиплексирует ARP-кадр и вызывает внутренний обработчик arp_process_packet.
 *
 * @param packet Инкапсулированный Ethernet-пакет с ARP-заголовком
 * @return status_t Код результата обработки
 */
/**
 * @brief Handle an incoming ARP Ethernet frame
 *
 * Extracts the ARP packet from the Ethernet frame,
 * validates it, learns the sender’s mapping, and
 * generates any necessary ARP replies.
 *
 * @param packet Pointer to the incoming Ethernet packet containing the ARP payload
 * @return status_t STATUS_SUCCESS if the ARP frame was processed successfully,
 *                  or an appropriate error code otherwise
 */
status_t arp_handle_frame(packet_buffer_t *packet);


/**
 * @brief Flush all entries from the ARP table
 * 
 * @param table Pointer to the ARP table structure
 * @return status_t Status of the operation
 */
status_t arp_flush(arp_table_t *table);

/**
 * @brief Age the entries in the ARP table and remove expired ones
 * 
 * @param table Pointer to the ARP table structure
 * @return status_t Status of the operation
 */
status_t arp_age_entries(arp_table_t *table);

/**
 * @brief Get ARP statistics
 * 
 * @param table Pointer to the ARP table structure
 * @param stats Pointer to store the statistics
 * @return status_t Status of the operation
 */
status_t arp_get_stats(arp_table_t *table, arp_stats_t *stats);

/**
 * @brief Set the timeout for ARP cache entries
 * 
 * @param table Pointer to the ARP table structure
 * @param timeout_seconds Timeout in seconds
 * @return status_t Status of the operation
 */
status_t arp_set_timeout(arp_table_t *table, uint32_t timeout_seconds);

/**
 * @brief Get all entries from the ARP table
 * 
 * @param table Pointer to the ARP table structure
 * @param entries Array to store the entries
 * @param max_entries Maximum number of entries to retrieve
 * @param num_entries Pointer to store the actual number of entries retrieved
 * @return status_t Status of the operation
 */
status_t arp_get_all_entries(arp_table_t *table, arp_entry_info_t *entries, 
                            uint16_t max_entries, uint16_t *num_entries);

//////**
///// * @brief Resolve MAC address for a given IP through ARP
///// *
///// * @param ip_addr Pointer to IPv4 address to resolve
///// * @param mac_addr Pointer to store resulting MAC address
///// * @return status_t Status code (STATUS_SUCCESS or error code)
///// */
/////status_t arp_resolve_next_hop(const ipv4_addr_t *ip_addr, mac_addr_t *mac_addr);
///
////**
/// * @brief Resolve MAC address for a given IP through ARP
/// *
/// * @param table Pointer to the ARP table structure
/// * @param ip_addr Pointer to IPv4 address to resolve
/// * @param mac_addr Pointer to store resulting MAC address
/// * @return status_t Status of the operation
/// */
///status_t arp_resolve_next_hop(arp_table_t *table, const ipv4_addr_t *ip_addr, mac_addr_t *mac_addr);
//status_t arp_resolve_next_hop(const ipv4_addr_t *ip_addr, mac_addr_t *mac_addr);
/**
 * @brief Resolve MAC address for a given IP through ARP on a specific port
 *
 * @param ip_addr    Указатель на структуру ipv4_addr_t (IP для разрешения)
 * @param port_index Индекс порта, через который шлем ARP-запрос
 * @param mac_addr   Куда записывать найденный MAC
 * @return status_t  STATUS_SUCCESS, ARP_STATUS_PENDING или иной код ошибки
 */
status_t arp_resolve_next_hop (const ipv4_addr_t *ip_addr, uint16_t port_index, mac_addr_t *mac_addr) ;

/**
 * @brief Get MAC address for a given IP address (wrapper function)
 *
 * This is a convenience function that uses the global ARP table instance
 * to lookup MAC address for a given IP. Used for compatibility with
 * ip_processing module.
 *
 * @param ip_addr IPv4 address (as uint32_t in network byte order)
 * @param mac_addr Pointer to store the found MAC address
 * @return status_t Status of the operation (STATUS_SUCCESS or error code)
 */
//status_t arp_get_mac_for_ip(uint32_t ip_addr, mac_addr_t *mac_addr);
/**
 * @brief Wrapper to resolve MAC via ARP on a specific port
 *
 * @param ip_addr    Указатель на структуру ipv4_addr_t (IP для разрешения)
 * @param port_index Индекс порта, через который шлем ARP-запрос
 * @param mac_addr   Куда записывать найденный MAC
 * @return status_t  STATUS_SUCCESS, ARP_STATUS_PENDING или иной код ошибки
 */
status_t arp_get_mac_for_ip (const ipv4_addr_t *ip_addr, uint16_t port_index, mac_addr_t *mac_addr) ;



status_t arp_resolve_async(const ipv4_addr_t *target_ip, uint16_t port_index);



#endif /* SDK_ES_SWITCH_SIMULATOR_INCLUDE_L3_ARP_H */

