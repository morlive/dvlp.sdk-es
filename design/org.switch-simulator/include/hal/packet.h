/**
 * @file packet.h
 * @brief Packet processing interface for switch simulator
 */

#ifndef SWITCH_SIM_PACKET_H
#define SWITCH_SIM_PACKET_H

#ifndef THREAD_LOCAL
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        #define THREAD_LOCAL _Thread_local
    #elif defined(__GNUC__) || defined(__clang__)
        #define THREAD_LOCAL __thread
    #elif defined(_MSC_VER)
        #define THREAD_LOCAL __declspec(thread)
    #else
        #define THREAD_LOCAL
        // Определим макрос который можно проверить
        #define NO_THREAD_LOCAL_SUPPORT
    #endif
#endif

#include "../common/types.h"
#include "../common/error_codes.h"

#define MAX_PACKET_SIZE 9216  // Support for jumbo frames

// option 1:
#define packet_alloc(size) packet_buffer_alloc(size)

typedef uint16_t ethertype_t;  /**< Ethernet frame type */

/* Ethernet protocol types (Ethertype) */
#define ETHERTYPE_IP     0x0800  /**< IPv4 */
#define ETHERTYPE_ARP    0x0806  /**< ARP */
#define ETHERTYPE_RARP   0x8035  /**< Reverse ARP */
#define ETHERTYPE_IPV6   0x86DD  /**< IPv6 */
#define ETHERTYPE_VLAN   0x8100  /**< IEEE 802.1Q VLAN tagging */
#define ETHERTYPE_QINQ   0x88A8  /**< IEEE 802.1ad (Q-in-Q) */
#define ETHERTYPE_MPLS   0x8847  /**< MPLS unicast */
#define ETHERTYPE_MPLS_MC 0x8848 /**< MPLS multicast */
#define ETHERTYPE_LLDP   0x88CC  /**< Link Layer Discovery Protocol */
#define ETHERTYPE_PTP    0x88F7  /**< Precision Time Protocol */
#define ETHERTYPE_FCOE   0x8906  /**< Fibre Channel over Ethernet */
#define ETHERTYPE_FIP    0x8914  /**< FCoE Initialization Protocol */
#define ETHERTYPE_ROCE   0x8915  /**< RDMA over Converged Ethernet */
#define ETHERTYPE_ISIS   0x8870  /**< IS-IS */
#define ETHERTYPE_JUMBO  0x8870  /**< Jumbo frames */
#define ETHERTYPE_LOOPBACK 0x9000 /**< Ethernet loopback */
#define ETHERTYPE_PPP    0x880B  /**< Point-to-Point Protocol */
#define ETHERTYPE_FLOW_CONTROL 0x8808 /**< IEEE 802.3x flow control */
#define ETHERTYPE_LACP   0x8809  /**< Link Aggregation Control Protocol */
#define ETHERTYPE_MACSEC 0x88E5  /**< MAC Security */
#define ETHERTYPE_PROFINET 0x8892 /**< PROFINET */
#define ETHERTYPE_WAKE_ON_LAN 0x0842 /**< Wake-on-LAN */

/* Макросы для работы с Ethertype */
#define IS_IP_ETHERTYPE(type) ((type) == ETHERTYPE_IP || (type) == ETHERTYPE_IPV6)

/**
 * @brief Packet direction
 */
typedef enum {
    PACKET_DIR_RX = 0,    /**< Received packet */
    PACKET_DIR_TX,        /**< Transmitted packet */
    PACKET_DIR_INTERNAL,  /**< Internally generated packet */
    PACKET_DIR_INVALID    /**< Invalid/uninitialized packet direction */
} packet_direction_t;

/**
 * @brief Packet metadata structure
 */
typedef struct {
    port_id_t port;              /**< Source/destination port */
    packet_direction_t direction; /**< Packet direction */
    vlan_id_t vlan;              /**< VLAN ID */
    uint8_t priority;            /**< Priority/CoS value */
    mac_addr_t src_mac;          /**< Source MAC address */
    mac_addr_t dst_mac;          /**< Destination MAC address */
    uint16_t ethertype;          /**< Ethertype */
    bool is_tagged;              /**< VLAN tagged flag */
    bool is_dropped;             /**< Packet drop flag */
    uint32_t timestamp;          /**< Packet timestamp */
} packet_metadata_t;

/**
 * @brief Packet buffer structure
 */
typedef struct {
    uint8_t *data;                  /**< Pointer to packet data */
    union {
      uint32_t length;              /**< Current length of packet data */
      uint32_t size;                /**< Current size of packet data */
    };
    uint32_t capacity;              /**< Total capacity of buffer */
    packet_metadata_t metadata;     /**< Packet metadata */
    void *user_data;                /**< User data pointer */
} packet_buffer_t;
// Создаем псевдоним для packet_buffer_t
typedef packet_buffer_t packet_t;

/**
 * @brief Packet processing result codes
 */
typedef enum {
    PACKET_RESULT_FORWARD = 0,   /**< Forward packet normally */
    PACKET_RESULT_DROP,          /**< Drop packet */
    PACKET_RESULT_CONSUME,       /**< Packet consumed (e.g., by control plane) */
    PACKET_RESULT_RECIRCULATE    /**< Recirculate packet for additional processing */
} packet_result_t;

/**
 * @brief Ethernet header structure
 */
typedef struct {
    mac_addr_t dst_mac;  // Destination MAC address
    mac_addr_t src_mac;  // Source MAC address
    uint16_t   ethertype; // Ethernet type/length
} ethernet_header_t;


/**
 * @brief Extract Ethernet header from packet
 *
 * @param packet Packet buffer
 * @param eth_header Pointer to store pointer to header within packet data
 * @return status_t STATUS_SUCCESS on success
 */
status_t packet_get_ethernet_header(const packet_buffer_t *packet, ethernet_header_t **eth_header);

/**
 * @brief Extract VLAN ID from packet
 *
 * @param packet Pointer to packet buffer
 * @param vlan_id Output parameter to store VLAN ID
 * @return status_t STATUS_SUCCESS on success
 */
status_t packet_get_vlan_id(const packet_buffer_t *packet, vlan_id_t *vlan_id);


/**
 * @brief Packet processing callback function type
 *
 * @param packet Packet buffer to process
 * @param user_data User data supplied during callback registration
 * @return packet_result_t Processing result
 */
typedef packet_result_t (*packet_process_cb_t)(packet_buffer_t *packet, void *user_data);

/**
 * @brief Initialize packet processing subsystem
 * 
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_init(void);

/**
 * @brief Shutdown packet processing subsystem
 * 
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_shutdown(void);

/**
 * @brief Allocate a new packet buffer
 * 
 * @param size Initial size of packet data
 * @return packet_buffer_t* Newly allocated packet buffer or NULL if failed
 */
packet_buffer_t* packet_buffer_alloc(uint32_t size);



/**
 * @brief Создать новый пакет с capacity = MAX_PACKET_SIZE.
 * @param[out] out_pkt Указатель на указатель, куда положим результат.
 * @return STATUS_SUCCESS или код ошибки из packet_buffer_alloc.
 */
status_t packet_create(packet_buffer_t **out_pkt);

/**
 * @brief Уничтожить пакет и установить указатель в NULL.
 * @param[in,out] pkt Указатель на указатель на пакет.
 */
void packet_destroy(packet_buffer_t *pkt);








/**
 * @brief Free a previously allocated packet buffer
 * 
 * @param packet Packet buffer to free
 */
void packet_buffer_free(packet_buffer_t *packet);

/**
 * @brief Reset a packet buffer to its initial state
 *
 * Resets a packet buffer to its initial state by clearing the data buffer,
 * resetting the size to 0, and restoring metadata to default values.
 * The buffer capacity remains unchanged and the buffer can be reused.
 *
 * @param packet Pointer to the packet buffer to reset
 */
void packet_reset(packet_buffer_t *packet);

/**
 * @brief Append data to a packet buffer
 * @param packet Pointer to the packet buffer
 * @param data   Pointer to the data to append
 * @param length Number of bytes to append
 * @return STATUS_SUCCESS on success, error code on failure
 */
status_t packet_append_data(packet_buffer_t *packet, const uint8_t *data, uint32_t length);

/**
 * @brief      Peek a single byte from packet buffer at given offset
 *
 * @return     STATUS_SUCCESS               если успешно
 * @return     ERROR_INVALID_PARAMETER      если аргументы некорректны
 * @return     ERROR_PACKET_OPERATION_FAILED если offset выходит за пределы size
 */
status_t packet_peek_byte(const packet_buffer_t *packet, uint32_t offset, uint8_t *byte);

/**
 * @brief      Peek a block of data from packet buffer at given offset
 *
 * @return     STATUS_SUCCESS                если успешно
 * @return     ERROR_INVALID_PARAMETER       если переданы некорректные аргументы
 * @return     ERROR_PACKET_OPERATION_FAILED если диапазон выходит за пределы текущего размера пакета
 */
status_t packet_peek_data(const packet_buffer_t *packet, uint32_t offset, void *dest, uint32_t length);

status_t packet_copy_data(const packet_buffer_t *packet, uint32_t offset, void *dest, uint32_t length);

/**
 * @brief      Update (overwrite) a block of data in packet buffer at given offset
 *
 * @return     STATUS_SUCCESS                если успешно
 * @return     ERROR_INVALID_PARAMETER       если аргументы некорректны
 * @return     ERROR_PACKET_OPERATION_FAILED если запись выйдет за пределы текущего размера пакета
 */
status_t packet_update_data(packet_buffer_t *packet, uint32_t offset, const void *src, uint32_t length);



/**
 * @brief Clone a packet buffer
 * 
 * @param packet Source packet buffer
 * @return packet_buffer_t* New copy of packet buffer or NULL if failed
 */
packet_buffer_t* packet_buffer_clone(const packet_buffer_t *packet);

/**
 * @brief Resize packet buffer data section
 * 
 * @param packet Packet buffer to resize
 * @param new_size New size for packet data
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_buffer_resize(packet_buffer_t *packet, uint32_t new_size);

/**
 * @brief Register a packet processing callback
 * 
 * @param callback Processing callback function
 * @param priority Processing priority (lower numbers = higher priority)
 * @param user_data User data to pass to callback
 * @param[out] handle_out Handle for registered callback
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_register_processor(packet_process_cb_t callback, 
                                  uint32_t priority, 
                                  void *user_data, 
                                  uint32_t *handle_out);

/**
 * @brief Unregister a packet processing callback
 *
 * @param handle Handle of registered callback
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_unregister_processor(uint32_t handle);

/**
 * @brief Process a packet through registered processors
 *
 * @param packet Packet buffer to process
 * @return packet_result_t Final processing result
 */
packet_result_t packet_process(packet_buffer_t *packet);

/**
 * @brief Inject a packet into the switch processing pipeline
 *
 * @param packet Packet buffer to inject
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_inject(packet_buffer_t *packet);

/**
 * @brief Transmit a packet out of a specific port
 *
 * @param packet Packet buffer to transmit
 * @param port_id Destination port ID
 * @return status_t STATUS_SUCCESS if successful
 */
status_t packet_transmit(packet_buffer_t *packet, port_id_t port_id);

/**
 * @brief Check if a packet has a VLAN tag
 * @param packet Packet to check
 * @param vlan_id Pointer to store the VLAN ID if found
 * @return true if packet has VLAN tag, false otherwise
 */
bool packet_has_vlan_tag(const packet_buffer_t *packet, vlan_id_t *vlan_id);

/**
 * @brief Copy a packet from source to destination
 * @param packet Source packet
 * @param out_packet Destination packet
 * @return status_t Status code
 */
status_t packet_copy(const packet_buffer_t *packet, packet_buffer_t *out_packet);

/**
 * @brief Set VLAN tag in a packet (modify existing tag if present)
 * @param packet Source packet
 * @param vlan_id VLAN ID to set
 * @param out_packet Output packet with modified VLAN tag
 * @return status_t Status code
 */
status_t packet_set_vlan_tag(const packet_buffer_t *packet, vlan_id_t vlan_id, packet_buffer_t *out_packet);

/**
 * @brief Add VLAN tag to a packet that doesn't have one
 * @param packet Source packet
 * @param vlan_id VLAN ID to add
 * @param out_packet Output packet with added VLAN tag
 * @return status_t Status code
 */
status_t packet_add_vlan_tag(const packet_buffer_t *packet, vlan_id_t vlan_id, packet_buffer_t *out_packet);

/**
 * @brief Remove VLAN tag from a packet
 * @param packet Source packet with VLAN tag
 * @param out_packet Output packet without VLAN tag
 * @return status_t Status code
 */
status_t packet_remove_vlan_tag(const packet_buffer_t *packet, packet_buffer_t *out_packet);




#endif /* SWITCH_SIM_PACKET_H */

