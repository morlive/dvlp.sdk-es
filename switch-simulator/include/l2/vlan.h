/**
 * @file vlan.h
 * @brief VLAN management interface
 */
#ifndef SWITCH_SIM_VLAN_H
#define SWITCH_SIM_VLAN_H
/////////SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include "../common/types.h"
#include "../common/error_codes.h"
#include "../hal/port.h"

#define MAX_VLANS 4096
#define VLAN_NAME_MAX_LEN 32

#define VLAN_ID_DEFAULT 1      /**< Default VLAN ID (usually 1 for default/native VLAN) */
#define VLAN_ID_ALL 0xFFFF     /**< Special VLAN ID representing all VLANs */

/* VLAN constants */
#define VLAN_ID_MIN         1
#define VLAN_ID_MAX         4094
#define DEFAULT_VLAN        1
#define VLAN_ID_INVALID     0

typedef enum {
    VLAN_PORT_MODE_ACCESS = 0,  /**< Access port - untagged for one VLAN */
    VLAN_PORT_MODE_TRUNK,       /**< Trunk port - tagged for multiple VLANs */
    VLAN_PORT_MODE_HYBRID       /**< Hybrid port - both tagged and untagged VLANs */
} vlan_port_mode_t;

typedef enum {
    VLAN_MEMBER_TAGGED = 0,    /**< Tagged member - frames sent with VLAN tag */
    VLAN_MEMBER_UNTAGGED       /**< Untagged member - frames sent without VLAN tag */
} vlan_member_type_t;

typedef struct {
    vlan_id_t      vlan_id;                 /**< VLAN identifier (1-4095) */
    //char           name[32];              /**< VLAN name */
    char           name[VLAN_NAME_MAX_LEN]; /**< VLAN name */
    bool           is_active;               /**< VLAN active state */
    uint64_t       member_ports;            /**< Bitset of member ports (both tagged and untagged) */
    uint64_t       untagged_ports;          /**< Bitset of untagged member ports */
    bool           learning_enabled;        /**< MAC learning enabled on this VLAN */
    bool           stp_enabled;             /**< STP enabled on this VLAN */
} vlan_entry_t;

typedef struct {
    vlan_port_mode_t mode;          /**< Port VLAN mode */
    vlan_id_t      pvid;            /**< Port VLAN ID (for ingress untagged frames) */
    vlan_id_t      native_vlan;     /**< Native VLAN for trunk ports (usually same as PVID) */
    bool           accept_untag;    /**< Accept untagged frames */
    bool           accept_tag;      /**< Accept tagged frames */
    bool           ingress_filter;  /**< Enable ingress filtering */
} vlan_port_config_t;

status_t vlan_init(uint32_t num_ports);

status_t vlan_deinit(void);

status_t vlan_create(vlan_id_t vlan_id, const char *name);

status_t vlan_delete(vlan_id_t vlan_id);

status_t vlan_get(vlan_id_t vlan_id, vlan_entry_t *entry);

status_t vlan_set_name(vlan_id_t vlan_id, const char *name);

status_t vlan_add_port(vlan_id_t vlan_id, port_id_t port_id, vlan_member_type_t member_type);

status_t vlan_remove_port(vlan_id_t vlan_id, port_id_t port_id);

status_t vlan_set_active(vlan_id_t vlan_id, bool active);

status_t vlan_set_learning(vlan_id_t vlan_id, bool enable);

status_t vlan_set_stp(vlan_id_t vlan_id, bool enable);

status_t vlan_set_port_config(port_id_t port_id, vlan_port_config_t *config);

status_t vlan_get_port_config(port_id_t port_id, vlan_port_config_t *config);

status_t vlan_get_all(vlan_entry_t *entries, uint32_t max_entries, uint32_t *count);

status_t vlan_get_by_port(port_id_t port_id, vlan_id_t *vlan_ids, uint32_t max_vlans, uint32_t *count);

typedef enum {
    VLAN_TAG_ACTION_NONE = 0,   /**< Don't modify VLAN tag */
    VLAN_TAG_ACTION_ADD,        /**< Add VLAN tag */
    VLAN_TAG_ACTION_REMOVE,     /**< Remove VLAN tag */
    VLAN_TAG_ACTION_REPLACE     /**< Replace VLAN tag */
} vlan_tag_action_t;

status_t vlan_process_packet(packet_info_t *packet_info, 
                           port_id_t in_port, 
                           port_id_t out_port,
                           vlan_id_t *out_vlan_id,
                           vlan_tag_action_t *tag_action);

typedef enum {
    VLAN_EVENT_CREATE = 0,      /**< VLAN created */
    VLAN_EVENT_DELETE,          /**< VLAN deleted */
    VLAN_EVENT_PORT_ADDED,      /**< Port added to VLAN */
    VLAN_EVENT_PORT_REMOVED,    /**< Port removed from VLAN */
    VLAN_EVENT_CONFIG_CHANGE    /**< VLAN configuration changed */
} vlan_event_type_t;

typedef void (*vlan_event_callback_t)(vlan_id_t vlan_id, 
                                    vlan_event_type_t event_type, 
                                    port_id_t port_id,  /* Valid for port events */
                                    void *user_data);

status_t vlan_register_event_callback(vlan_event_callback_t callback, void *user_data);


/**
 * @brief Process a packet for VLAN tagging/untagging on egress
 *
 * @param packet Packet to process
 * @param vlan_id VLAN ID associated with the packet
 * @param out_port Port on which the packet will be sent
 * @param out_packet Output packet after VLAN processing
 * @return status_t Status code
 */
status_t vlan_process_egress(const packet_buffer_t *packet, vlan_id_t vlan_id, port_id_t out_port, packet_buffer_t *out_packet);


#endif /* SWITCH_SIM_VLAN_H */
