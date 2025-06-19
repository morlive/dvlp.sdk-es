/**
 * @file arp.c
 * @brief Implementation of ARP (Address Resolution Protocol) functionality
 *
 * This file contains the implementation of ARP table management and related operations,
 * including ARP cache maintenance, lookups, and packet processing.
 */

#include "l3/arp.h"
#include "l3/ip_processing.h"
#include "l2/mac_table.h"
#include "l2/vlan.h"
#include "common/logging.h"
#include "common/error_codes.h"
#include "hal/packet.h"
#include "hal/port.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/* Defines */
#define ARP_CACHE_SIZE 1024
#define ARP_CACHE_TIMEOUT_SEC 1200  /* 20 minutes by default */
#define ARP_REQUEST_RETRY_COUNT 3
#define ARP_REQUEST_RETRY_INTERVAL_MS 1000

/* ARP packet format definitions */
#define ARP_HARDWARE_TYPE_ETHERNET 1
#define ARP_PROTOCOL_TYPE_IPV4 0x0800
#define ARP_HARDWARE_SIZE_ETHERNET 6
#define ARP_PROTOCOL_SIZE_IPV4 4
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

// В начале файла arp.c (после включения заголовочных файлов)
static arp_table_t g_arp_table; // Глобальная переменная для ARP-таблицы

/* ARP packet structure */
typedef struct /* __attribute__((__packed__)) */ 
{
    uint16_t hw_type;       /* Hardware type (Ethernet = 1) */
    uint16_t protocol_type; /* Protocol type (IPv4 = 0x0800) */
    uint8_t hw_addr_len;    /* Hardware address length (6 for MAC) */
    uint8_t proto_addr_len; /* Protocol address length (4 for IPv4) */
    uint16_t operation;     /* Operation (1=request, 2=reply) */
    mac_addr_t sender_mac;  /* Sender MAC address */
    ipv4_addr_t sender_ip;  /* Sender IP address */
    mac_addr_t target_mac;  /* Target MAC address */
    ipv4_addr_t target_ip;  /* Target IP address */
} arp_packet_t;

/* ARP cache entry states */
typedef enum {
    ARP_STATE_INCOMPLETE,  /* Resolution in progress */
    ARP_STATE_REACHABLE,   /* Confirmed reachability */
    ARP_STATE_STALE,       /* Reachability requires confirmation */
    ARP_STATE_DELAY,       /* Waiting before sending probe */
    ARP_STATE_PROBE,       /* Actively probing */
    ARP_STATE_FAILED       /* ARP resolution failed */
} arp_state_t;

/* ARP cache entry structure */
typedef struct arp_entry {
    ipv4_addr_t ip;           /* IPv4 address */
    mac_addr_t mac;           /* MAC address */
    arp_state_t state;        /* Entry state */
    uint32_t created_time;    /* Creation timestamp */
    uint32_t updated_time;    /* Last update timestamp */
    uint16_t port_index;      /* Port where this MAC was learned */
    uint8_t retry_count;      /* Retry counter for ARP requests */
    struct arp_entry *next;   /* Pointer for hash collision resolution */
} arp_entry_t;

/* ARP table structure */
struct arp_table_s {
    arp_entry_t *hash_table[ARP_CACHE_SIZE]; /* Hash table buckets */
    arp_entry_t *entry_pool;                 /* Pre-allocated entries pool */
    uint16_t entry_count;                    /* Number of entries in use */
    uint32_t timeout;                        /* ARP cache timeout in seconds */
    bool initialized;                        /* Initialization flag */
    arp_stats_t stats;                       /* ARP statistics */
} ;

///typedef struct {
///    arp_entry_t *hash_table[ARP_CACHE_SIZE]; /* Hash table buckets */
///    arp_entry_t *entry_pool;                 /* Pre-allocated entries pool */
///    uint16_t entry_count;                    /* Number of entries in use */
///    uint32_t timeout;                        /* ARP cache timeout in seconds */
///    bool initialized;                        /* Initialization flag */
///    arp_stats_t stats;                       /* ARP statistics */
///} arp_table_t;

/* Function prototypes for internal use */
static uint32_t hash_ipv4(const ipv4_addr_t *ipv4);
static arp_entry_t *arp_find_entry(arp_table_t *table, const ipv4_addr_t *ipv4);
static arp_entry_t *arp_allocate_entry(arp_table_t *table);
static void arp_free_entry(arp_table_t *table, arp_entry_t *entry);
static status_t arp_send_request(arp_table_t *table, const ipv4_addr_t *target_ip, uint16_t port_index);
static status_t arp_send_reply(arp_table_t *table, const ipv4_addr_t *target_ip, const mac_addr_t *target_mac,
                               const ipv4_addr_t *sender_ip, uint16_t port_index);
static status_t arp_process_packet(arp_table_t *table, const packet_buffer_t *packet, uint16_t port_index);
static uint32_t get_current_time(void);

/* Convert internal state to public API state */
static uint8_t arp_state_to_public(arp_state_t state) {
    switch (state) {
        case ARP_STATE_INCOMPLETE: return ARP_ENTRY_STATE_INCOMPLETE;
        case ARP_STATE_REACHABLE:  return ARP_ENTRY_STATE_REACHABLE;
        case ARP_STATE_STALE:      return ARP_ENTRY_STATE_STALE;
        case ARP_STATE_DELAY:      return ARP_ENTRY_STATE_DELAY;
        case ARP_STATE_PROBE:      return ARP_ENTRY_STATE_PROBE;
        case ARP_STATE_FAILED:     return ARP_ENTRY_STATE_FAILED;
        default:                   return ARP_ENTRY_STATE_FAILED;
    }
}


// Функция-геттер для получения указателя на ARP-таблицу
arp_table_t* arp_table_get_instance(void) {
    return &g_arp_table;
}

/**
 * @brief Initialize the ARP module
 *
 * @param table Pointer to ARP table structure
 * @return status_t Status code indicating success or failure
 */
status_t arp_init(arp_table_t *table) {
    if (!table) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: NULL ARP table pointer");
        return STATUS_INVALID_PARAMETER;
    }

    LOG_INFO( LOG_CATEGORY_L3, "Initializing ARP module");

    /* Clear the ARP table structure */
    memset(table, 0, sizeof(arp_table_t));

    /* Pre-allocate ARP entries */
    table->entry_pool = (arp_entry_t *)calloc(ARP_CACHE_SIZE, sizeof(arp_entry_t));
    if (!table->entry_pool) {
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate memory for ARP cache entries");
        return STATUS_NO_MEMORY;
    }

    /* Initialize ARP cache timeout */
    table->timeout = ARP_CACHE_TIMEOUT_SEC;
    table->initialized = true;

    LOG_INFO( LOG_CATEGORY_L3, "ARP module initialized successfully, cache size: %d entries", ARP_CACHE_SIZE);
    return STATUS_SUCCESS;
}

/**
 * @brief Clean up the ARP module resources
 *
 * @param table Pointer to ARP table structure
 * @return status_t Status code indicating success or failure
 */
status_t arp_deinit(arp_table_t *table) {
    if (!table) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: NULL ARP table pointer");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
         LOG_WARNING( LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_INFO( LOG_CATEGORY_L3, "Cleaning up ARP module resources");

    /* Free the entry pool */
    if (table->entry_pool) {
        free(table->entry_pool);
        table->entry_pool = NULL;
    }

    /* Reset table structure */
    memset(table, 0, sizeof(arp_table_t));

    LOG_INFO( LOG_CATEGORY_L3, "ARP module resources cleaned up successfully");
    return STATUS_SUCCESS;
}

/**
 * @brief Add or update an entry in the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @param ipv4 IPv4 address
 * @param mac MAC address
 * @param port_index Port index where the MAC was learned
 * @return status_t Status code indicating success or failure
 */
status_t arp_add_entry(arp_table_t *table, const ipv4_addr_t *ipv4, const mac_addr_t *mac, uint16_t port_index) {
    if (!table || !ipv4 || !mac) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_add_entry");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_DEBUG( LOG_CATEGORY_L3, "Adding/updating ARP entry for IP: %d.%d.%d.%d", 
              (*ipv4 >> 24) & 0xFF ,
              (*ipv4 >> 16) & 0xFF ,
              (*ipv4 >> 8)  & 0xFF , 
              (*ipv4 )      & 0xFF
              );

    /* Look for existing entry */
    arp_entry_t *entry = arp_find_entry(table, ipv4);
    
    if (entry) {
        /* Update existing entry */
        memcpy(&entry->mac, mac, sizeof(mac_addr_t));
        entry->port_index = port_index;
        entry->updated_time = get_current_time();
        entry->state = ARP_STATE_REACHABLE;
        
        LOG_DEBUG( LOG_CATEGORY_L3, "Updated existing ARP entry");
    } else {
        /* Allocate new entry */
        entry = arp_allocate_entry(table);
        if (!entry) {
            LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate new ARP entry");
            return STATUS_RESOURCE_EXHAUSTED;
        }
        
        /* Initialize new entry */
        memcpy(&entry->ip, ipv4, sizeof(ipv4_addr_t));
        memcpy(&entry->mac, mac, sizeof(mac_addr_t));
        entry->port_index = port_index;
        entry->created_time = get_current_time();
        entry->updated_time = entry->created_time;
        entry->state = ARP_STATE_REACHABLE;
        entry->retry_count = 0;
        
        /* Add to hash table */
        uint32_t hash = hash_ipv4(ipv4) % ARP_CACHE_SIZE;
        entry->next = table->hash_table[hash];
        table->hash_table[hash] = entry;
        table->entry_count++;
        
        LOG_DEBUG( LOG_CATEGORY_L3, "Added new ARP entry, current count: %d", table->entry_count);
    }
    
    /* Update MAC table as well to ensure L2 forwarding works properly */
    /* This is assuming we have a function to update the MAC table */
    
    mac_table_add(*mac, port_index, VLAN_ID_DEFAULT, false);
    
    table->stats.entries_added++;
    return STATUS_SUCCESS;
}

/**
 * @brief Look up an entry in the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @param ipv4 IPv4 address to look up
 * @param mac_result Pointer to store the resulting MAC address
 * @param port_index_result Pointer to store the resulting port index
 * @return status_t Status code indicating success or failure
 */
status_t arp_lookup(arp_table_t *table, const ipv4_addr_t *ipv4, mac_addr_t *mac_result, uint16_t *port_index_result) {
    if (!table || !ipv4 || !mac_result) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_lookup");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_DEBUG(  LOG_CATEGORY_L3, "Looking up ARP entry for IP: %d.%d.%d.%d",
                (*ipv4 >> 24) & 0xFF,
                (*ipv4 >> 16) & 0xFF,
                (*ipv4 >> 8)  & 0xFF,
                (*ipv4 )      & 0xFF
              );
              //ipv4->bytes[0], ipv4->bytes[1], ipv4->bytes[2], ipv4->bytes[3]);

    /* Find entry in the cache */
    arp_entry_t *entry = arp_find_entry(table, ipv4);
    
    if (!entry) {
        LOG_DEBUG( LOG_CATEGORY_L3, "ARP entry not found, initiating resolution");
        
        /* If the entry doesn't exist, start the resolution process */
        /* Determine the outgoing interface using the routing table */
        uint16_t out_port = 0; /* This should be determined using the routing table */
        
        /* Create incomplete entry */
        arp_entry_t *new_entry = arp_allocate_entry(table);
        if (!new_entry) {
            LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate new ARP entry for resolution");
            return STATUS_RESOURCE_EXHAUSTED;
        }
        
        /* Initialize incomplete entry */
        memcpy(&new_entry->ip, ipv4, sizeof(ipv4_addr_t));
        memset(&new_entry->mac, 0, sizeof(mac_addr_t));
        new_entry->port_index = out_port;
        new_entry->created_time = get_current_time();
        new_entry->updated_time = new_entry->created_time;
        new_entry->state = ARP_STATE_INCOMPLETE;
        new_entry->retry_count = 0;
        
        /* Add to hash table */
        uint32_t hash = hash_ipv4(ipv4) % ARP_CACHE_SIZE;
        new_entry->next = table->hash_table[hash];
        table->hash_table[hash] = new_entry;
        table->entry_count++;
        
        /* Send ARP request */
        arp_send_request(table, ipv4, out_port);
        table->stats.requests_sent++;
        
        return ARP_STATUS_PENDING;
    }
    
    /* Entry exists, check if it's complete */
    if (entry->state != ARP_STATE_REACHABLE) {
        if (entry->state == ARP_STATE_INCOMPLETE) {
            LOG_DEBUG( LOG_CATEGORY_L3, "ARP resolution in progress");
            return ARP_STATUS_PENDING;
        } else if (entry->state == ARP_STATE_FAILED) {
            LOG_DEBUG( LOG_CATEGORY_L3, "ARP resolution previously failed");
            return STATUS_NOT_FOUND;
        }
    }
    
    /* Return valid entry */
    memcpy(mac_result, &entry->mac, sizeof(mac_addr_t));
    if (port_index_result) {
        *port_index_result = entry->port_index;
    }
    
    /* Update statistics */
    table->stats.cache_hits++;
    
    LOG_DEBUG( LOG_CATEGORY_L3, "ARP entry found");
    return STATUS_SUCCESS;
}

/**
 * @brief Remove an entry from the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @param ipv4 IPv4 address of the entry to remove
 * @return status_t Status code indicating success or failure
 */
status_t arp_remove_entry(arp_table_t *table, const ipv4_addr_t *ipv4) {
    if (!table || !ipv4) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_remove_entry");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_DEBUG( LOG_CATEGORY_L3, "Removing ARP entry for IP: %d.%d.%d.%d", 
                (*ipv4 >> 24) & 0xFF,
                (*ipv4 >> 16) & 0xFF,
                (*ipv4 >> 8)  & 0xFF,
                (*ipv4 )      & 0xFF
              );

    uint32_t hash = hash_ipv4(ipv4) % ARP_CACHE_SIZE;
    arp_entry_t *entry = table->hash_table[hash];
    arp_entry_t *prev = NULL;
    
    /* Search for the entry in the hash chain */
    while (entry) {
        if (memcmp(&entry->ip, ipv4, sizeof(ipv4_addr_t)) == 0) {
            /* Found the entry, remove it from the hash chain */
            if (prev) {
                prev->next = entry->next;
            } else {
                table->hash_table[hash] = entry->next;
            }
            
            /* Free the entry */
            arp_free_entry(table, entry);
            table->entry_count--;
            table->stats.entries_removed++;
            
            LOG_DEBUG( LOG_CATEGORY_L3, "ARP entry removed, current count: %d", table->entry_count);
            return STATUS_SUCCESS;
        }
        
        prev = entry;
        entry = entry->next;
    }
    
     LOG_DEBUG( LOG_CATEGORY_L3, "ARP entry not found for removal");
    return STATUS_NOT_FOUND;
}

/**
 * @brief Process an incoming ARP packet
 *
 * @param table Pointer to ARP table structure
 * @param packet Pointer to the packet structure
 * @param port_index Port index where the packet was received
 * @return status_t Status code indicating success or failure
 */
// status_t arp_process_packet(arp_table_t *table, const packet_buffer_t *packet, uint16_t port_index) {
static status_t arp_process_packet(arp_table_t *table, const packet_buffer_t *packet, uint16_t port_index) {
    if (!table || !packet) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_process_packet");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_DEBUG( LOG_CATEGORY_L3, "Processing ARP packet received on port %d", port_index);

    /* Verify packet size */
    if (packet->size < sizeof(arp_packet_t)) {
         LOG_WARNING( LOG_CATEGORY_L3, "Received ARP packet is too small: %d bytes", packet->size);
        table->stats.invalid_packets++;
        return STATUS_INVALID_PACKET;
    }
    
    /* Parse ARP packet */
    const arp_packet_t *arp_packet = (const arp_packet_t *)packet->data;
    
    /* Verify packet fields */
    if (ntohs(arp_packet->hw_type) != ARP_HARDWARE_TYPE_ETHERNET ||
        ntohs(arp_packet->protocol_type) != ARP_PROTOCOL_TYPE_IPV4 ||
        arp_packet->hw_addr_len != ARP_HARDWARE_SIZE_ETHERNET ||
        arp_packet->proto_addr_len != ARP_PROTOCOL_SIZE_IPV4) {
        
         LOG_WARNING( LOG_CATEGORY_L3, "Invalid ARP packet format");
        table->stats.invalid_packets++;
        return STATUS_INVALID_PACKET;
    }
    
    uint16_t operation = ntohs(arp_packet->operation);
    
    /* Learn sender's IP-to-MAC mapping regardless of packet type */
    ipv4_addr_t sender_ip;
    mac_addr_t sender_mac;
    memcpy(&sender_ip, &arp_packet->sender_ip, sizeof(ipv4_addr_t));
    memcpy(&sender_mac, &arp_packet->sender_mac, sizeof(mac_addr_t));
    
    /* Update ARP cache with sender's info */
    arp_add_entry(table, &sender_ip, &sender_mac, port_index);
    
    /* Process based on ARP operation */
    switch (operation) {
        case ARP_OP_REQUEST: {
            LOG_DEBUG( LOG_CATEGORY_L3, "Received ARP request");
            table->stats.requests_received++;
            
            /* Extract target IP */
            ipv4_addr_t target_ip;
            memcpy(&target_ip, &arp_packet->target_ip, sizeof(ipv4_addr_t));
            
            /* Check if we can respond (if we own this IP) */
            bool is_our_ip = false; /* This should check against our interfaces */
            
            if (is_our_ip) {
                /* Get our MAC address for this interface */
                mac_addr_t our_mac __attribute__((unused)); 
                /* TODO: Implement interface MAC lookup functionality */
                /* This should be a lookup to get the MAC of the target interface */
                
                /* Send ARP reply */
                arp_send_reply(table, &sender_ip, &sender_mac, &target_ip, port_index);
                table->stats.replies_sent++;
            }
            break;
        }
        
        case ARP_OP_REPLY: {
            LOG_DEBUG( LOG_CATEGORY_L3, "Received ARP reply");
            table->stats.replies_received++;
            
            /* ARP entry already updated from sender info above */
            break;
        }
        
        default:
             LOG_WARNING( LOG_CATEGORY_L3, "Unknown ARP operation: %d", operation);
            table->stats.invalid_packets++;
            return STATUS_INVALID_PACKET;
    }
    
    return STATUS_SUCCESS;
}

status_t arp_handle_frame(packet_buffer_t *packet) {
    
    // Получаем указатель на внутреннюю таблицу ARP
    extern arp_table_t g_arp_table;  // или через функцию arp_table_get_instance()
    // arp_table_t *table = arp_table_get_instance();     // Optional

    return arp_process_packet(&g_arp_table, packet, packet->metadata.port);
}

/**
 * @brief Flush all entries from the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @return status_t Status code indicating success or failure
 */
status_t arp_flush(arp_table_t *table) {
    if (!table) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: NULL ARP table pointer");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_INFO( LOG_CATEGORY_L3, "Flushing ARP cache");

    /* Free all entries in hash buckets */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_entry_t *entry = table->hash_table[i];
        
        while (entry) {
            arp_entry_t *next = entry->next;
            arp_free_entry(table, entry);
            entry = next;
        }
        
        table->hash_table[i] = NULL;
    }
    
    table->entry_count = 0;
    table->stats.cache_flushes++;
    
    LOG_INFO( LOG_CATEGORY_L3, "ARP cache flushed successfully");
    return STATUS_SUCCESS;
}

/**
 * @brief Age out old entries from the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @return status_t Status code indicating success or failure
 */
status_t arp_age_entries(arp_table_t *table) {
    if (!table) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: NULL ARP table pointer");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_DEBUG( LOG_CATEGORY_L3, "Aging ARP cache entries");

    uint32_t current_time = get_current_time();
    uint32_t aged_count = 0;
    
    /* Check all entries in hash buckets */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_entry_t *entry = table->hash_table[i];
        arp_entry_t *prev = NULL;
        
        while (entry) {
            arp_entry_t *next = entry->next;
            
            /* Check if entry has timed out */
            if (entry->state == ARP_STATE_REACHABLE &&
                (current_time - entry->updated_time) > table->timeout) {
                
                /* Remove from hash chain */
                if (prev) {
                    prev->next = next;
                } else {
                    table->hash_table[i] = next;
                }
                
                /* Free the entry */
                arp_free_entry(table, entry);
                table->entry_count--;
                aged_count++;
                
                /* Don't update prev since we removed the entry */
            } else {
                /* Handle incomplete entries that have timed out */
                if (entry->state == ARP_STATE_INCOMPLETE) {
                    if ((current_time - entry->updated_time) > ARP_REQUEST_RETRY_INTERVAL_MS / 1000) {
                        if (entry->retry_count < ARP_REQUEST_RETRY_COUNT) {
                            /* Retry ARP request */
                            arp_send_request(table, &entry->ip, entry->port_index);
                            entry->retry_count++;
                            entry->updated_time = current_time;
                            table->stats.requests_sent++;
                        } else {
                            /* Max retries reached, mark as failed */
                            entry->state = ARP_STATE_FAILED;
                        }
                    }
                }
                
                prev = entry;
            }
            
            entry = next;
        }
    }
    
    if (aged_count > 0) {
        LOG_DEBUG( LOG_CATEGORY_L3, "Aged out %d ARP entries", aged_count);
        table->stats.entries_aged += aged_count;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get ARP statistics
 *
 * @param table Pointer to ARP table structure
 * @param stats Pointer to store statistics
 * @return status_t Status code indicating success or failure
 */
status_t arp_get_stats(arp_table_t *table, arp_stats_t *stats) {
    if (!table || !stats) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_get_stats");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    /* Update current entry count in stats */
    table->stats.current_entries = table->entry_count;
    
    /* Copy statistics */
    memcpy(stats, &table->stats, sizeof(arp_stats_t));
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set the ARP cache timeout value
 *
 * @param table Pointer to ARP table structure
 * @param timeout_seconds Timeout value in seconds
 * @return status_t Status code indicating success or failure
 */
status_t arp_set_timeout(arp_table_t *table, uint32_t timeout_seconds) {
    if (!table) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: NULL ARP table pointer");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    LOG_INFO( LOG_CATEGORY_L3, "Setting ARP cache timeout to %d seconds", timeout_seconds);
    
    table->timeout = timeout_seconds;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get all entries in the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @param entries Array to store entries
 * @param max_entries Maximum number of entries to retrieve
 * @param num_entries Pointer to store the actual number of entries retrieved
 * @return status_t Status code indicating success or failure
 */
status_t arp_get_all_entries(arp_table_t *table, arp_entry_info_t *entries, 
                            uint16_t max_entries, uint16_t *num_entries) {
    if (!table || !entries || !num_entries) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_get_all_entries");
        return STATUS_INVALID_PARAMETER;
    }

    if (!table->initialized) {
        LOG_ERROR(LOG_CATEGORY_L3, "ARP module not initialized");
        return STATUS_NOT_INITIALIZED;
    }

    uint16_t count = 0;
    
    /* Iterate through all hash buckets */
    for (int i = 0; i < ARP_CACHE_SIZE && count < max_entries; i++) {
        arp_entry_t *entry = table->hash_table[i];
        
        while (entry && count < max_entries) {
            /* Copy entry information */
            memcpy(&entries[count].ip, &entry->ip, sizeof(ipv4_addr_t));
            memcpy(&entries[count].mac, &entry->mac, sizeof(mac_addr_t));
            entries[count].port_index = entry->port_index;
            entries[count].state = arp_state_to_public(entry->state);
            entries[count].age = get_current_time() - entry->updated_time;
            
            count++;
            entry = entry->next;
        }
    }
    
    *num_entries = count;
    
    LOG_DEBUG( LOG_CATEGORY_L3, "Retrieved %d ARP entries", count);
    return STATUS_SUCCESS;
}

/**
 * @brief Resolve MAC address for a given IP through ARP
 *
 * This function resolves a MAC address for the provided IP address
 * using ARP protocol. It will check the ARP cache first, and if not found,
 * will initiate an ARP request.
 *
 * @param ip_addr Pointer to IPv4 address to resolve
 * @param mac_addr Pointer to store resulting MAC address
 * @return status_t Status code (STATUS_SUCCESS or error code)
 */
//status_t arp_resolve_next_hop(const ipv4_addr_t *ip_addr, mac_addr_t *mac_addr) {
/**
 * @brief Resolve MAC address for a given IP through ARP on a specific port
 *
 * @param ip_addr    Указатель на структуру ipv4_addr_t (IP для разрешения)
 * @param port_index Индекс порта, через который шлем ARP-запрос
 * @param mac_addr   Куда записывать найденный MAC
 * @return status_t  STATUS_SUCCESS, ARP_STATUS_PENDING или иной код ошибки
 */
status_t arp_resolve_next_hop(const ipv4_addr_t *ip_addr, uint16_t port_index, mac_addr_t *mac_addr)
{
//    uint16_t port_index;
//    arp_table_t *table = arp_table_get_instance();
    
    if (!ip_addr || !mac_addr) {
        return ERROR_INVALID_PARAMETER;
    }
    
    arp_table_t *table = arp_table_get_instance();
    uint16_t dummy_port;
    // Check if entry exists in ARP cache
    status_t status = arp_lookup(table, ip_addr, mac_addr, &dummy_port);
    
    if (status == STATUS_SUCCESS) {
        return STATUS_SUCCESS;
    } else if (status == ARP_STATUS_CACHE_MISS) {
        // Здесь должен быть код для инициирования ARP-запроса
        // // шлём ARP-запрос на нужном порту
        arp_send_request(table, ip_addr, port_index);
        return ARP_STATUS_PENDING;
    }
    
    return status;
}

/**
 * @brief Lookup or resolve MAC for given IP via ARP.
 *
 * @param ip_addr   Указатель на структуру ipv4_addr_t.
 * @param mac_addr  Буфер mac_addr_t для записи найденного MAC.
 * @return status_t STATUS_SUCCESS, ARP_STATUS_PENDING или другой код ошибки.
 */
//status_t arp_get_mac_for_ip(const ipv4_addr_t *ip_addr, mac_addr_t *mac_addr)
/**
 * @brief Wrapper to resolve MAC via ARP on a specific port
 *
 * @param ip_addr    Указатель на структуру ipv4_addr_t (IP для разрешения)
 * @param port_index Индекс порта, через который шлем ARP-запрос
 * @param mac_addr   Куда записывать найденный MAC 
 * @return status_t  STATUS_SUCCESS, ARP_STATUS_PENDING или иной код ошибки
 */
status_t arp_get_mac_for_ip(const ipv4_addr_t *ip_addr, uint16_t port_index, mac_addr_t *mac_addr)
{
    if (!ip_addr || !mac_addr) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    // просто перекидываем в arp_resolve_next_hop,
    // или сразу дублируем логику кеш-проверки:
    //return arp_resolve_next_hop(ip_addr, mac_addr);
    return arp_resolve_next_hop(ip_addr, port_index, mac_addr);
}


///// old
/////**
//// * @brief Get MAC address for a given IP address (wrapper function)
//// * 
//// * This is a convenience function that uses the global ARP table instance
//// * to lookup MAC address for a given IP. Used for compatibility with 
//// * ip_processing module.
//// *
//// * @param ip_addr IPv4 address (as uint32_t in network byte order)
//// * @param mac_addr Pointer to store the found MAC address
//// * @return status_t Status of the operation (STATUS_SUCCESS or error code)
//// */
////status_t arp_get_mac_for_ip(uint32_t ip_addr, mac_addr_t *mac_addr) {
////    if (!mac_addr) {
////        return STATUS_ERROR_INVALID_PARAM;
////    }
////
////    // Получаем глобальный экземпляр ARP таблицы
////    arp_table_t *table = arp_table_get_instance();
////    if (!table) {
////        return STATUS_ERROR_NOT_INITIALIZED;
////    }
////
////    // Конвертируем uint32_t в ipv4_addr_t
////    ipv4_addr_t ipv4_addr;
////    ipv4_addr.addr = ip_addr;
////
////    // Используем существующую функцию lookup
////    uint16_t port_index_result;
////    status_t result = arp_lookup(table, &ipv4_addr, mac_addr, &port_index_result);
////
////    if (result == STATUS_SUCCESS) {
////        return STATUS_SUCCESS;
////    }
////
////    // Если не найден, попробуем разрешить через ARP
////    result = arp_resolve_next_hop(&ipv4_addr, mac_addr);
////    return result;
////}




//// НОВАЯ ПУБЛИЧНАЯ ФУНКЦИЯ
//status_t arp_resolve_async(const ipv4_addr_t *target_ip, uint16_t port_index) {
//    if (!target_ip) {
//        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: target_ip is NULL");
//        return STATUS_INVALID_PARAMETER;
//    }
//
//    // Получаем доступ к ARP-таблице
//    arp_table_t *table = get_arp_table(); // Или get_arp_table_for_port(port_index)
//    if (!table) {
//        LOG_ERROR(LOG_CATEGORY_L3, "Failed to get ARP table");
//        return STATUS_ERROR;
//    }
//
//    LOG_DEBUG(LOG_CATEGORY_L3, "Initiating ARP resolution for IP: %d.%d.%d.%d on port %d",
//              (*target_ip >> 24) & 0xFF, (*target_ip >> 16) & 0xFF,
//              (*target_ip >> 8) & 0xFF, (*target_ip) & 0xFF, port_index);
//
//    // Можно добавить проверки на rate limiting, дублирование запросов и т.д.
//
//    return arp_send_request(table, target_ip, port_index);
//}

// Публичная обёртка
status_t arp_resolve_async(const ipv4_addr_t *target_ip, uint16_t port_index)
{
    if (!target_ip) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter: target_ip is NULL");
        return STATUS_INVALID_PARAMETER;
    }

    // Используем геттер для единственной ARP-таблицы
    arp_table_t *table = arp_table_get_instance();
    if (!table) {
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to get ARP table instance");
        return STATUS_ERROR;
    }

    LOG_DEBUG(LOG_CATEGORY_L3,
              "Initiating ARP resolution for IP: %d.%d.%d.%d on port %d",
              (*target_ip >> 24) & 0xFF,
              (*target_ip >> 16) & 0xFF,
              (*target_ip >> 8)  & 0xFF,
              (*target_ip)       & 0xFF,
              port_index);

    return arp_send_request(table, target_ip, port_index);
}





/**
 * @brief Calculate hash for an IPv4 address
 *
 * @param ipv4 Pointer to IPv4 address
 * @return uint32_t Hash value
 */
static uint32_t hash_ipv4(const ipv4_addr_t *ipv4) {
    uint32_t ip_value;
    memcpy(&ip_value, ipv4, sizeof(ipv4_addr_t));
    
    /* A simple hash function based on multiplication and XOR */
    uint32_t hash = ip_value;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = (hash >> 16) ^ hash;
    
    return hash;
}

/**
 * @brief Find an entry in the ARP cache
 *
 * @param table Pointer to ARP table structure
 * @param ipv4 IPv4 address to find
 * @return arp_entry_t* Pointer to the entry if found, NULL otherwise
 */
static arp_entry_t *arp_find_entry(arp_table_t *table, const ipv4_addr_t *ipv4) {
    uint32_t hash = hash_ipv4(ipv4) % ARP_CACHE_SIZE;
    arp_entry_t *entry = table->hash_table[hash];
    
    while (entry) {
        if (memcmp(&entry->ip, ipv4, sizeof(ipv4_addr_t)) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief Allocate a new ARP entry from the pool
 *
 * @param table Pointer to ARP table structure
 * @return arp_entry_t* Pointer to the allocated entry, NULL if pool is exhausted
 */
static arp_entry_t *arp_allocate_entry(arp_table_t *table) {
    /* If we've reached the maximum number of entries, recycle the oldest one */
    if (table->entry_count >= ARP_CACHE_SIZE) {
        uint32_t oldest_time = UINT32_MAX;
        arp_entry_t *oldest_entry = NULL;
        int oldest_bucket = -1;
        arp_entry_t *oldest_prev = NULL;
        
        /* Find the oldest entry */
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            arp_entry_t *entry = table->hash_table[i];
            arp_entry_t *prev = NULL;
            
            while (entry) {
                if (entry->updated_time < oldest_time) {
                    oldest_time = entry->updated_time;
                    oldest_entry = entry;
                    oldest_bucket = i;
                    oldest_prev = prev;
                }
                
                prev = entry;
                entry = entry->next;
            }
        }
        
        /* Remove the oldest entry from its hash chain */
        if (oldest_entry) {
            if (oldest_prev) {
                oldest_prev->next = oldest_entry->next;
            } else {
                table->hash_table[oldest_bucket] = oldest_entry->next;
            }
            
            /* Clear the entry for reuse */
            memset(oldest_entry, 0, sizeof(arp_entry_t));
            table->entry_count--; /* Will be incremented again when entry is added */
            
            return oldest_entry;
        }
        
        /* If we couldn't find an entry to recycle, fail */
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate ARP entry: cache full and no entry available for recycling");
        return NULL;
    }
    
    /* Allocate a new entry from the pool */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_entry_t *entry = &table->entry_pool[i];
        
        /* Find a free entry (not linked in any hash bucket) */
        bool is_used = false;
        for (int j = 0; j < ARP_CACHE_SIZE; j++) {
            arp_entry_t *bucket_entry = table->hash_table[j];
            
            while (bucket_entry) {
                if (bucket_entry == entry) {
                    is_used = true;
                    break;
                }
                bucket_entry = bucket_entry->next;
            }
            
            if (is_used) {
                break;
            }
        }
        
        if (!is_used) {
            /* Found a free entry */
            memset(entry, 0, sizeof(arp_entry_t));
            return entry;
        }
    }
    
    LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate ARP entry: all entries in use");
    return NULL;
}

/**
 * @brief Free an ARP entry back to the pool
 *
 * @param table Pointer to ARP table structure
 * @param entry Pointer to the entry to free
 */
static void arp_free_entry(arp_table_t *table, arp_entry_t *entry) {
    /* Clear the entry */
    memset(entry, 0, sizeof(arp_entry_t));
    /* Note: We don't need to do anything else since the entry is already
     * removed from its hash chain by the caller */
}

/**
 * @brief Send an ARP request packet
 *
 * @param table Pointer to ARP table structure
 * @param target_ip Target IPv4 address to resolve
 * @param port_index Port index to send the request on
 * @return status_t Status code indicating success or failure
 */
static status_t arp_send_request(arp_table_t *table, const ipv4_addr_t *target_ip, uint16_t port_index) {
    if (!table || !target_ip) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_send_request");
        return STATUS_INVALID_PARAMETER;
    }
    
    LOG_DEBUG(LOG_CATEGORY_L3, "Sending ARP request for IP: %d.%d.%d.%d on port %d", 
              (*target_ip >> 24) & 0xFF, (*target_ip >> 16) & 0xFF,
              (*target_ip >> 8) & 0xFF, (*target_ip) & 0xFF, port_index);

    /* Get our IP and MAC addresses for the outgoing interface */
    ipv4_addr_t our_ip;
    mac_addr_t our_mac;
    
    /* This should be a lookup to get our interface info */
    /* For now, using placeholder data */
    
    /* Create ARP packet payload */
    arp_packet_t arp_packet;
    
    /* Fill in ARP packet fields */
    arp_packet.hw_type = htons(ARP_HARDWARE_TYPE_ETHERNET);
    arp_packet.protocol_type = htons(ARP_PROTOCOL_TYPE_IPV4);
    arp_packet.hw_addr_len = ARP_HARDWARE_SIZE_ETHERNET;
    arp_packet.proto_addr_len = ARP_PROTOCOL_SIZE_IPV4;
    arp_packet.operation = htons(ARP_OP_REQUEST);
    
    /* Sender details (our info) */
    memcpy(&arp_packet.sender_mac, &our_mac, sizeof(mac_addr_t));
    memcpy(&arp_packet.sender_ip, &our_ip, sizeof(ipv4_addr_t));
    
    /* Target details */
    memset(&arp_packet.target_mac, 0, sizeof(mac_addr_t)); /* Unknown MAC */
    memcpy(&arp_packet.target_ip, target_ip, sizeof(ipv4_addr_t));
    
    /* Allocate buffer for packet (Ethernet header + ARP payload) */
    const uint32_t total_size = sizeof(ethernet_header_t) + sizeof(arp_packet_t);
    packet_buffer_t *packet = packet_buffer_alloc(total_size);
    if (!packet) {
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate packet buffer for ARP request");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Set Ethernet broadcast destination for ARP request */
    mac_addr_t broadcast_mac = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    
    /* Construct Ethernet header */
    ethernet_header_t eth_header;
    memcpy(&eth_header.src_mac, &our_mac, sizeof(mac_addr_t));
    memcpy(&eth_header.dst_mac, &broadcast_mac, sizeof(mac_addr_t));
    eth_header.ethertype = htons(ETHERTYPE_ARP);
    
    /* Assemble complete packet: Ethernet header + ARP payload */
    memcpy(packet->data, &eth_header, sizeof(ethernet_header_t));
    memcpy(packet->data + sizeof(ethernet_header_t), &arp_packet, sizeof(arp_packet_t));
    packet->size = total_size;
    
    /* Set packet metadata */
    packet->metadata.port = port_index;
    packet->metadata.direction = PACKET_DIR_TX;
    packet->metadata.timestamp = 0; /* Real implementation would set current time */
    
    /* Transmit packet */
    status_t status = packet_transmit(packet, port_index);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to transmit ARP request: %s", 
                  error_to_string(status));
    } else {
        LOG_DEBUG(LOG_CATEGORY_L3, "ARP request transmitted successfully");
    }
    
    /* Free packet resources */
    packet_buffer_free(packet);
    
    return status;
}

/**
 * @brief Send an ARP reply packet
 *
 * @param table Pointer to ARP table structure
 * @param target_ip Target IPv4 address
 * @param target_mac Target MAC address
 * @param sender_ip Sender IPv4 address
 * @param port_index Port index to send the reply on
 * @return status_t Status code indicating success or failure
 */
static status_t arp_send_reply(arp_table_t *table, const ipv4_addr_t *target_ip, const mac_addr_t *target_mac,
                              const ipv4_addr_t *sender_ip, uint16_t port_index) {
    if (!table || !target_ip || !target_mac || !sender_ip) {
        LOG_ERROR(LOG_CATEGORY_L3, "Invalid parameter(s) in arp_send_reply");
        return STATUS_INVALID_PARAMETER;
    }

    LOG_DEBUG(  LOG_CATEGORY_L3, "Sending ARP request for IP: %d.%d.%d.%d on port %d",
                (*target_ip >> 24) & 0xFF,
                (*target_ip >> 16) & 0xFF,
                (*target_ip >> 8)  & 0xFF,
                (*target_ip)       & 0xFF,
                port_index
              );

    /* Get our MAC address for the interface */
    mac_addr_t our_mac;

    /* This should be a lookup to get our interface MAC */
    /* For now, using placeholder data */

    /* Create ARP reply packet */
    arp_packet_t arp_packet;

    /* Fill in ARP packet fields */
    arp_packet.hw_type = htons(ARP_HARDWARE_TYPE_ETHERNET);
    arp_packet.protocol_type = htons(ARP_PROTOCOL_TYPE_IPV4);
    arp_packet.hw_addr_len = ARP_HARDWARE_SIZE_ETHERNET;
    arp_packet.proto_addr_len = ARP_PROTOCOL_SIZE_IPV4;
    arp_packet.operation = htons(ARP_OP_REPLY);

    /* Sender details (our info) */
    memcpy(&arp_packet.sender_mac, &our_mac, sizeof(mac_addr_t));
    memcpy(&arp_packet.sender_ip, sender_ip, sizeof(ipv4_addr_t));

    /* Target details */
    memcpy(&arp_packet.target_mac, target_mac, sizeof(mac_addr_t));
    memcpy(&arp_packet.target_ip, target_ip, sizeof(ipv4_addr_t));

    /* Create packet to carry ARP reply */
    packet_buffer_t *packet;

    /* Allocate packet buffer */
    packet = packet_buffer_alloc(sizeof(arp_packet_t));
    if (!packet) {
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to allocate packet for ARP reply");
        return STATUS_MEMORY_ALLOCATION_FAILED;
    }

    /* Copy ARP packet into packet data */
    memcpy(packet->data, &arp_packet, sizeof(arp_packet_t));
    packet->size = sizeof(arp_packet_t);

    /* Send packet directly to the target MAC */
    //status_t status = port_send_packet(port_index, packet, &our_mac, target_mac, ETHERTYPE_ARP);
    status_t status = port_send_packet_ext(port_index, packet, &our_mac, target_mac, ETHERTYPE_ARP);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_L3, "Failed to send ARP reply packet");
    }

    /* Free packet resources */
    packet_buffer_free(packet);

    return status;
}


/**
 * @brief Get current system time in seconds
 *
 * @return uint32_t Current time in seconds
 */
static uint32_t get_current_time(void) {
    /* This should use a system-specific function to get the current time */
    /* For now, returning a simple counter that increments with each call */
    static uint32_t time_counter = 0;
    return time_counter++;
}
