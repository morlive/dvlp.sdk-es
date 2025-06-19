/**
 * @file stp.c
 * @brief Implementation of Spanning Tree Protocol functionality
 *
 * This file implements the Spanning Tree Protocol for the switch simulator,
 * including BPDU processing, port state management, and topology calculation.
 */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common/types.h"
#include "common/error_codes.h"
#include "common/logging.h"
#include "common/threading.h"
#include "hal/port.h"
#include "hal/packet.h"
#include "l2/stp.h"
#include "l2/vlan.h"

/** temporary for solve
 * @brief Определение для обратной совместимости со старым наименованием типа
 */
typedef packet_buffer_t packet_t;

/**
 * @brief Default STP parameters
 */
#define STP_DEFAULT_BRIDGE_PRIORITY 32768
#define STP_DEFAULT_PORT_PRIORITY 128
#define STP_DEFAULT_PATH_COST 19     // Default cost for 100Mbps link
#define STP_DEFAULT_HELLO_TIME 2     // 2 seconds
#define STP_DEFAULT_MAX_AGE 20       // 20 seconds
#define STP_DEFAULT_FORWARD_DELAY 15 // 15 seconds

#define STP_LOCK() spinlock_acquire(&g_stp_bridge.lock)
#define STP_UNLOCK() spinlock_release(&g_stp_bridge.lock)

#define STP_HELLO_TIMER 2
#define STP_TCN_TIMER 1

#define STP_BPDU_CONFIG 0x00
#define STP_BPDU_TCN 0x80

#define STP_FLAG_TC 0x01        // Topology Change flag
#define STP_FLAG_TCA 0x80       // Topology Change Acknowledgment flag

typedef struct {
    port_id_t port_id;             // Port ID
    stp_port_state_t state;        // Port state (disabled, blocking, listening, learning, forwarding)
    uint16_t port_priority;        // Port priority
    uint32_t path_cost;            // Path cost
    bridge_id_t designated_root;   // Root bridge ID
    uint32_t root_path_cost;       // Root path cost
    bridge_id_t designated_bridge; // Designated bridge ID
    port_id_t designated_port;     // Designated port ID
    uint32_t message_age;          // Message age
    uint32_t max_age;              // Max age
    uint32_t hello_time;           // Hello time
    uint32_t forward_delay;        // Forward delay
    bool topology_change;          // Topology change flag
    bool topology_change_ack;      // Topology change acknowledgment flag
    uint32_t timer_hello;          // Hello timer
    uint32_t timer_tcn;            // Topology change notification timer
    uint32_t timer_forward_delay;  // Forward delay timer
    uint32_t timer_message_age;    // Message age timer
    bool bpdu_received;            // Whether BPDU received on this port
    // Информация, специфичная для VLAN
    struct stp_vlan_info {
        stp_port_state_t state;    // State for this VLAN
    // Другие поля, которые могут различаться для разных VLAN
    } *vlans;                      // Массив состояний для разных VLAN
    uint16_t vlans_allocated;      // Количество выделенной памяти для VLAN
} stp_port_info_t;

typedef struct {
    bool enabled;                  // STP enabled flag
    bridge_id_t bridge_id;         // Bridge ID (priority + MAC)
    bridge_id_t root_id;           // Root bridge ID
    uint32_t root_path_cost;       // Root path cost
    port_id_t root_port;           // Root port ID
    uint32_t max_age;              // Max age
    uint32_t hello_time;           // Hello time
    uint32_t forward_delay;        // Forward delay
    bool topology_change;          // Topology change flag
    uint32_t topology_change_time; // Topology change time
    uint32_t timer_hello;          // Hello timer
    uint32_t timer_topology_change; // Topology change timer
    uint32_t ports_count;          // Number of ports
    stp_port_info_t *ports;        // Array of port info
    spinlock_t lock;               // Lock for thread-safe access
} stp_bridge_info_t;

static stp_bridge_info_t g_stp_bridge;

static inline void stp_acquire_lock(void) {
      spinlock_acquire(&g_stp_bridge.lock);
}

static inline void stp_release_lock(void) {
      spinlock_release(&g_stp_bridge.lock);
}

static int compare_bridge_id(bridge_id_t id1, bridge_id_t id2) {
    // Compare bridge priorities first (lower is better)
    if (id1.priority < id2.priority) {
        return -1;
    } else if (id1.priority > id2.priority) {
        return 1;
    }
    
    // If priorities equal, compare MAC addresses (lower is better)
    return memcmp(id1.mac_address.addr, id2.mac_address.addr, MAC_ADDR_LEN);
}

static status_t generate_bpdu(port_id_t port_id, uint8_t bpdu_type, packet_buffer_t *packet) {
    if (!packet) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid packet buffer");
        return ERROR_INVALID_PARAMETER;
    }
    
    if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
        return ERROR_INVALID_STATE;
    }
    
    stp_port_info_t *port = &g_stp_bridge.ports[port_id];
    
    // Fill in BPDU header
    packet->data[0] = 0x42;  // BPDU destination MAC (01:80:C2:00:00:00)
    packet->data[1] = 0x42;
    packet->data[2] = 0x03;
    packet->data[3] = 0x00;
    packet->data[4] = 0x00;
    packet->data[5] = 0x00;
    
    // Source MAC is the bridge MAC
    memcpy(&packet->data[6], &g_stp_bridge.bridge_id.mac_address, MAC_ADDR_LEN);
    
    // LLC/SNAP header
    packet->data[12] = 0x00;  // Length
    packet->data[13] = 0x26;  // Length for config BPDU (38 bytes)
    packet->data[14] = 0x42;  // DSAP
    packet->data[15] = 0x42;  // SSAP
    packet->data[16] = 0x03;  // Control
    
    // BPDU header
    packet->data[17] = 0x00;  // Protocol identifier (0x0000)
    packet->data[18] = 0x00;
    packet->data[19] = 0x00;  // Protocol version (0x00)
    packet->data[20] = bpdu_type;  // BPDU type
    
    if (bpdu_type == STP_BPDU_CONFIG) {
        // Config BPDU fields
        uint8_t flags = 0;
        if (port->topology_change) {
            flags |= STP_FLAG_TC;
        }
        if (port->topology_change_ack) {
            flags |= STP_FLAG_TCA;
        }
        
        packet->data[21] = flags;  // Flags
        
        // Root ID
        packet->data[22] = (g_stp_bridge.root_id.priority >> 8) & 0xFF;
        packet->data[23] = g_stp_bridge.root_id.priority & 0xFF;
        memcpy(&packet->data[24], &g_stp_bridge.root_id.mac_address, MAC_ADDR_LEN);
        
        // Root path cost
        packet->data[30] = (g_stp_bridge.root_path_cost >> 24) & 0xFF;
        packet->data[31] = (g_stp_bridge.root_path_cost >> 16) & 0xFF;
        packet->data[32] = (g_stp_bridge.root_path_cost >> 8) & 0xFF;
        packet->data[33] = g_stp_bridge.root_path_cost & 0xFF;
        
        // Bridge ID
        packet->data[34] = (g_stp_bridge.bridge_id.priority >> 8) & 0xFF;
        packet->data[35] = g_stp_bridge.bridge_id.priority & 0xFF;
        memcpy(&packet->data[36], &g_stp_bridge.bridge_id.mac_address, MAC_ADDR_LEN);
        
        // Port ID
        packet->data[42] = (port->port_priority >> 8) & 0xFF;
        packet->data[43] = port_id & 0xFF;
        
        // Message age
        uint16_t message_age = port->message_age * 256;
        packet->data[44] = (message_age >> 8) & 0xFF;
        packet->data[45] = message_age & 0xFF;
        
        // Max age
        uint16_t max_age = g_stp_bridge.max_age * 256;
        packet->data[46] = (max_age >> 8) & 0xFF;
        packet->data[47] = max_age & 0xFF;
        
        // Hello time
        uint16_t hello_time = g_stp_bridge.hello_time * 256;
        packet->data[48] = (hello_time >> 8) & 0xFF;
        packet->data[49] = hello_time & 0xFF;
        
        // Forward delay
        uint16_t forward_delay = g_stp_bridge.forward_delay * 256;
        packet->data[50] = (forward_delay >> 8) & 0xFF;
        packet->data[51] = forward_delay & 0xFF;
        
        packet->size = 52;  // Total length of config BPDU
    } else if (bpdu_type == STP_BPDU_TCN) {
        // TCN BPDU is much simpler
        packet->data[13] = 0x03;  // Length for TCN BPDU (3 bytes)
        packet->size = 21;  // Total length of TCN BPDU
    }
    
    return STATUS_SUCCESS;
}

static void stp_reconfigure_topology(void) {
   // Set all ports to blocking except root port
   for (uint32_t i = 0; i < g_stp_bridge.ports_count; i++) {
       if (i == g_stp_bridge.root_port) {
           // Root port should be forwarding
           stp_port_info_t *port = &g_stp_bridge.ports[i];
           
           if (port->state == STP_PORT_STATE_BLOCKING) {
               port->state = STP_PORT_STATE_LISTENING;
               port->timer_forward_delay = g_stp_bridge.forward_delay;
               LOG_INFO(LOG_CATEGORY_L2, "Root port %u transitions from blocking to listening", i);
           }
       } else {
           // Check if this port should be designated port
           stp_port_info_t *port = &g_stp_bridge.ports[i];
           bool is_designated = false;
           
           // If we're the root bridge, all ports are designated
           if (compare_bridge_id(g_stp_bridge.root_id, g_stp_bridge.bridge_id) == 0) {
               is_designated = true;
           } else {
               // Check if this port provides the best path to the root for its segment
               // (This is a simplified check that needs to be expanded in real implementation)
               if (port->bpdu_received && 
                   compare_bridge_id(g_stp_bridge.root_id, port->designated_root) < 0) {
                   is_designated = true;
               }
           }
           
           if (is_designated) {
               if (port->state == STP_PORT_STATE_BLOCKING) {
                   port->state = STP_PORT_STATE_LISTENING;
                   port->timer_forward_delay = g_stp_bridge.forward_delay;
                   LOG_INFO(LOG_CATEGORY_L2, "Designated port %u transitions from blocking to listening", i);
               }
           } else {
               if (port->state != STP_PORT_STATE_BLOCKING) {
                   port->state = STP_PORT_STATE_BLOCKING;
                   LOG_INFO(LOG_CATEGORY_L2, "Port %u transitions to blocking", i);
               }
           }
       }
   }
}

static status_t process_bpdu(port_id_t port_id, const packet_t *packet) {
    if (!packet) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid packet buffer");
        return ERROR_INVALID_PARAMETER;
    }
    
    if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
        return ERROR_INVALID_STATE;
    }
    
    // Validate minimum BPDU length
    if (packet->size < 21) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid BPDU length %u", packet->size);
        return ERROR_INVALID_PACKET;
    }
    
    // Check BPDU type
    uint8_t bpdu_type = packet->data[20];
    stp_port_info_t *port = &g_stp_bridge.ports[port_id];
    
    if (bpdu_type == STP_BPDU_CONFIG) {
        // Process config BPDU
        if (packet->size < 52) {
            LOG_ERROR(LOG_CATEGORY_L2, "Invalid config BPDU length %u", packet->size);
            return ERROR_INVALID_PACKET;
        }
        
        // Extract BPDU fields
        uint8_t flags = packet->data[21];
        
        // Extract root bridge ID
        bridge_id_t received_root_id;
        received_root_id.priority = (packet->data[22] << 8) | packet->data[23];
        memcpy(&received_root_id.mac_address, &packet->data[24], MAC_ADDR_LEN);
        
        // Extract root path cost
        uint32_t received_root_path_cost = (packet->data[30] << 24) | 
                                          (packet->data[31] << 16) | 
                                          (packet->data[32] << 8) | 
                                           packet->data[33];
        
        // Extract sending bridge ID
        bridge_id_t received_bridge_id;
        received_bridge_id.priority = (packet->data[34] << 8) | packet->data[35];
        memcpy(received_bridge_id.mac_address.addr, &packet->data[36], MAC_ADDR_LEN);
        
        // Extract port ID
        port_id_t received_port_id = (packet->data[42] << 8) | packet->data[43];
        
        // Extract timers
        uint16_t received_message_age = ((packet->data[44] << 8) | packet->data[45]) / 256;
        uint16_t received_max_age = ((packet->data[46] << 8) | packet->data[47]) / 256;
        uint16_t received_hello_time = ((packet->data[48] << 8) | packet->data[49]) / 256;
        uint16_t received_forward_delay = ((packet->data[50] << 8) | packet->data[51]) / 256;
        
        // Mark that we've received a BPDU on this port
        port->bpdu_received = true;
        
        // Update topology change flags
        if (flags & STP_FLAG_TC) {
            g_stp_bridge.topology_change = true;
            g_stp_bridge.topology_change_time = STP_DEFAULT_FORWARD_DELAY * 2;
        }
        
        // Check if this is a superior BPDU (better root or same root with better path)
        int root_comparison = compare_bridge_id(received_root_id, g_stp_bridge.root_id);
        bool is_superior = false;
        if (root_comparison < 0) {
           // Received root is better than our current root
           is_superior = true;
       } else if (root_comparison == 0) {
           // Same root, compare path cost
           if (received_root_path_cost < g_stp_bridge.root_path_cost) {
               is_superior = true;
           } else if (received_root_path_cost == g_stp_bridge.root_path_cost) {
               // Same path cost, compare sending bridge ID
               int bridge_comparison = compare_bridge_id(received_bridge_id, g_stp_bridge.bridge_id);
               if (bridge_comparison < 0) {
                   is_superior = true;
               } else if (bridge_comparison == 0) {
                   // Same bridge (should not happen unless we're getting our own BPDU back)
                   // Compare port IDs
                   if (received_port_id < port_id) {
                       is_superior = true;
                   }
               }
           }
       }
       
       if (is_superior) {
           // Update root bridge info
           g_stp_bridge.root_id = received_root_id;
           g_stp_bridge.root_path_cost = received_root_path_cost + port->path_cost;
           g_stp_bridge.root_port = port_id;
           
           // Update timers from root
           g_stp_bridge.max_age = received_max_age;
           g_stp_bridge.hello_time = received_hello_time;
           g_stp_bridge.forward_delay = received_forward_delay;
           
           // Update port info
           port->designated_root = received_root_id;
           port->root_path_cost = received_root_path_cost;
           port->designated_bridge = received_bridge_id;
           port->designated_port = received_port_id;
           port->message_age = received_message_age;
           
           // Reset topology change notification
           port->timer_tcn = 0;
           
           // Start reconfiguration process
           stp_reconfigure_topology();
           
           LOG_INFO(LOG_CATEGORY_L2, "Superior BPDU received on port %u, new root bridge ID: %04x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                    port_id,
                    g_stp_bridge.root_id.priority,
                    g_stp_bridge.root_id.mac_address.addr[0],
                    g_stp_bridge.root_id.mac_address.addr[1],
                    g_stp_bridge.root_id.mac_address.addr[2],
                    g_stp_bridge.root_id.mac_address.addr[3],
                    g_stp_bridge.root_id.mac_address.addr[4],
                    g_stp_bridge.root_id.mac_address.addr[5]);
       } else {
           // Check if this port should be designated port
           if (compare_bridge_id(received_root_id, port->designated_root) > 0 ||
               (compare_bridge_id(received_root_id, port->designated_root) == 0 &&
                received_root_path_cost > port->root_path_cost) ||
               (compare_bridge_id(received_root_id, port->designated_root) == 0 &&
                received_root_path_cost == port->root_path_cost &&
                compare_bridge_id(received_bridge_id, port->designated_bridge) > 0) ||
               (compare_bridge_id(received_root_id, port->designated_root) == 0 &&
                received_root_path_cost == port->root_path_cost &&
                compare_bridge_id(received_bridge_id, port->designated_bridge) == 0 &&
                received_port_id > port->designated_port)) {
               
               // This port should be designated port
               if (port->state == STP_PORT_STATE_BLOCKING) {
                   // Move to listening state
                   port->state = STP_PORT_STATE_LISTENING;
                   port->timer_forward_delay = g_stp_bridge.forward_delay;
                   LOG_INFO(LOG_CATEGORY_L2, "Port %u transitions from blocking to listening", port_id);
               }
           } else {
               // This port should be blocked
               if (port->state != STP_PORT_STATE_BLOCKING) {
                   port->state = STP_PORT_STATE_BLOCKING;
                   LOG_INFO(LOG_CATEGORY_L2, "Port %u transitions to blocking", port_id);
               }
           }
       }
   } else if (bpdu_type == STP_BPDU_TCN) {
       // Process topology change notification BPDU
       LOG_INFO(LOG_CATEGORY_L2, "TCN BPDU received on port %u", port_id);
       
       // Set topology change flag
       g_stp_bridge.topology_change = true;
       g_stp_bridge.topology_change_time = STP_DEFAULT_FORWARD_DELAY * 2;
       
       // Set topology change acknowledgment in next BPDU
       port->topology_change_ack = true;
   } else {
       LOG_ERROR(LOG_CATEGORY_L2, "Unknown BPDU type %u", bpdu_type);
       return ERROR_INVALID_PACKET;
   }
   
   return STATUS_SUCCESS;
}

static void stp_update_port_states(uint32_t current_time) {
   static uint32_t last_time = 0;
   
   // If time hasn't advanced, nothing to do
   if (current_time == last_time) {
       return;
   }
   
   uint32_t elapsed = current_time - last_time;
   last_time = current_time;
   
   // Update bridge timers
   if (g_stp_bridge.timer_hello > 0) {
       if (g_stp_bridge.timer_hello <= elapsed) {
           g_stp_bridge.timer_hello = g_stp_bridge.hello_time;
           
           // Send BPDUs on all ports if we're the root bridge
           if (compare_bridge_id(g_stp_bridge.root_id, g_stp_bridge.bridge_id) == 0) {
               for (uint32_t i = 0; i < g_stp_bridge.ports_count; i++) {
                   stp_port_info_t *port = &g_stp_bridge.ports[i];
                   if (port->state != STP_PORT_STATE_DISABLED) {
                       packet_t bpdu_packet = {0};
                       generate_bpdu(i, STP_BPDU_CONFIG, &bpdu_packet);
                       //port_send_packet(i, &bpdu_packet);
                       packet_transmit(&bpdu_packet, i);
                   }
               }
           }
       } else {
           g_stp_bridge.timer_hello -= elapsed;
       }
   }
   
   if (g_stp_bridge.topology_change_time > 0) {
       if (g_stp_bridge.topology_change_time <= elapsed) {
           g_stp_bridge.topology_change_time = 0;
           g_stp_bridge.topology_change = false;
           LOG_INFO(LOG_CATEGORY_L2, "Topology change period ended");
       } else {
           g_stp_bridge.topology_change_time -= elapsed;
       }
   }
   
   // Update port timers and states
   for (uint32_t i = 0; i < g_stp_bridge.ports_count; i++) {
       stp_port_info_t *port = &g_stp_bridge.ports[i];
       
       // Skip disabled ports
       if (port->state == STP_PORT_STATE_DISABLED) {
           continue;
       }
       
       // Update message age timer
       if (port->timer_message_age > 0) {
           if (port->timer_message_age <= elapsed) {
               port->timer_message_age = 0;
               
               // Message age expiry means we lost contact with the root
               if (i == g_stp_bridge.root_port) {
                   LOG_INFO(LOG_CATEGORY_L2, "Message age timer expired on root port %u, electing new root", i);
                   
                   // Become root ourselves
                   g_stp_bridge.root_id = g_stp_bridge.bridge_id;
                   g_stp_bridge.root_path_cost = 0;
                   g_stp_bridge.root_port = PORT_ID_INVALID;
                   
                   // Reconfigure topology
                   stp_reconfigure_topology();
               }
           } else {
               port->timer_message_age -= elapsed;
           }
       }
       
       // Update TCN timer
       if (port->timer_tcn > 0) {
           if (port->timer_tcn <= elapsed) {
               port->timer_tcn = STP_TCN_TIMER;
               
               // Send TCN BPDU on the root port
               if (i == g_stp_bridge.root_port) {
                   packet_t tcn_packet = {0};
                   generate_bpdu(i, STP_BPDU_TCN, &tcn_packet);
                   // port_send_packet(i, &tcn_packet);
                   packet_transmit(&tcn_packet, i);
                   LOG_INFO(LOG_CATEGORY_L2, "Sent TCN BPDU on root port %u", i);
               }
           } else {
               port->timer_tcn -= elapsed;
           }
       }
       
       // Update forward delay timer
       if (port->timer_forward_delay > 0) {
           if (port->timer_forward_delay <= elapsed) {
               port->timer_forward_delay = 0;
               
               // State transitions
               if (port->state == STP_PORT_STATE_LISTENING) {
                   port->state = STP_PORT_STATE_LEARNING;
                   port->timer_forward_delay = g_stp_bridge.forward_delay;
                   LOG_INFO(LOG_CATEGORY_L2, "Port %u transitions from listening to learning", i);
               } else if (port->state == STP_PORT_STATE_LEARNING) {
                   port->state = STP_PORT_STATE_FORWARDING;
                   LOG_INFO(LOG_CATEGORY_L2, "Port %u transitions from learning to forwarding", i);
               }
           } else {
               port->timer_forward_delay -= elapsed;
           }
       }
   }
}

status_t stp_init(const stp_config_t *config ) {
   if (!config) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid parameters");
       return ERROR_INVALID_PARAMETER;
   }

    uint32_t num_ports = 0;
    status_t status = port_get_count(&num_ports);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_L2, "Failed to get port count");
        return status;
    }
   
   // Allocate memory for port information
   g_stp_bridge.ports = malloc(sizeof(stp_port_info_t) * num_ports);
   if (!g_stp_bridge.ports) {
       LOG_ERROR(LOG_CATEGORY_L2, "Failed to allocate memory for STP ports");
       return STATUS_MEMORY_ALLOCATION_FAILED;
   }
   
   // Initialize bridge
   g_stp_bridge.enabled = config->enabled;
   g_stp_bridge.bridge_id = config->bridge_id;
   
   // Initially, we consider ourselves the root
   g_stp_bridge.root_id = g_stp_bridge.bridge_id;
   g_stp_bridge.root_path_cost = 0;
   g_stp_bridge.root_port = PORT_ID_INVALID;
   
   // Set default timers
   g_stp_bridge.max_age = config->max_age;
   g_stp_bridge.hello_time = config->hello_time;
   g_stp_bridge.forward_delay = config->forward_delay;
   g_stp_bridge.timer_hello = config->hello_time;
   g_stp_bridge.topology_change = false;
   g_stp_bridge.topology_change_time = 0;
   g_stp_bridge.ports_count = num_ports;


   // Initialize ports
   for (uint32_t i = 0; i < num_ports; i++) {
       stp_port_info_t *port = &g_stp_bridge.ports[i];
       port->port_id = i;
       port->state = STP_PORT_STATE_BLOCKING;
       port->port_priority = STP_DEFAULT_PORT_PRIORITY;
       port->path_cost = STP_DEFAULT_PATH_COST;
       port->designated_root = g_stp_bridge.root_id;
       port->root_path_cost = 0;
       port->designated_bridge = g_stp_bridge.bridge_id;
       port->designated_port = i;
       port->message_age = 0;
       port->max_age = g_stp_bridge.max_age;
       port->hello_time = g_stp_bridge.hello_time;
       port->forward_delay = g_stp_bridge.forward_delay;
       port->topology_change = false;
       port->topology_change_ack = false;
       
       // Выделение памяти для информации о VLAN
       port->vlans = calloc(MAX_VLANS, sizeof(struct stp_vlan_info));
       if (!port->vlans) {
           LOG_ERROR(LOG_CATEGORY_L2, "Failed to allocate memory for STP VLAN states");
           // Освобождаем ранее выделенную память
           for (uint32_t j = 0; j < i; j++) {
               if (g_stp_bridge.ports[j].vlans) {
                   free(g_stp_bridge.ports[j].vlans);
               }
           }
           free(g_stp_bridge.ports);
           g_stp_bridge.ports = NULL;
           return STATUS_MEMORY_ALLOCATION_FAILED;
       }
       port->vlans_allocated = MAX_VLANS;

       // Инициализация состояний VLAN по умолчанию
       for (uint16_t j = 0; j < MAX_VLANS; j++) {
           port->vlans[j].state = port->state; // Используем то же состояние, что и у порта
       }

       port->timer_hello = 0;
       port->timer_tcn = 0;
       port->timer_forward_delay = 0;
       port->timer_message_age = 0;
       port->bpdu_received = false;
   }
   
   LOG_INFO(LOG_CATEGORY_L2, "STP initialized with bridge ID: %04x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            g_stp_bridge.bridge_id.priority,
            g_stp_bridge.bridge_id.mac_address.addr[0],
            g_stp_bridge.bridge_id.mac_address.addr[1],
            g_stp_bridge.bridge_id.mac_address.addr[2],
            g_stp_bridge.bridge_id.mac_address.addr[3],
            g_stp_bridge.bridge_id.mac_address.addr[4],
            g_stp_bridge.bridge_id.mac_address.addr[5]);

   return STATUS_SUCCESS;
}

status_t stp_deinit(void) {
   stp_acquire_lock();

   if (g_stp_bridge.ports) {
       // Освобождение памяти для VLAN в каждом порту
       for (uint32_t i = 0; i < g_stp_bridge.ports_count; i++) {
           if (g_stp_bridge.ports[i].vlans) {
               free(g_stp_bridge.ports[i].vlans);
               g_stp_bridge.ports[i].vlans = NULL;
           }
       }

       free(g_stp_bridge.ports);
       g_stp_bridge.ports = NULL;
   }

   g_stp_bridge.ports_count = 0;
   g_stp_bridge.enabled = false;

   stp_release_lock();

   LOG_INFO(LOG_CATEGORY_L2, "STP de-initialized");
   return STATUS_SUCCESS;
}

status_t stp_set_enabled(bool enable) {
   stp_acquire_lock();

   bool was_enabled = g_stp_bridge.enabled;
   g_stp_bridge.enabled = enable;

   if (!was_enabled && enable) {
       // Re-initialize ports to blocking state
       for (uint32_t i = 0; i < g_stp_bridge.ports_count; i++) {
           stp_port_info_t *port = &g_stp_bridge.ports[i];
           if (port->state != STP_PORT_STATE_DISABLED) {
               port->state = STP_PORT_STATE_BLOCKING;
           }
       }

       // If we're enabling STP, start the hello timer
       g_stp_bridge.timer_hello = 0;  // Trigger immediate BPDU transmission

       LOG_INFO(LOG_CATEGORY_L2, "STP enabled");
   } else if (was_enabled && !enable) {
       // If we're disabling STP, set all ports to forwarding
       for (uint32_t i = 0; i < g_stp_bridge.ports_count; i++) {
           stp_port_info_t *port = &g_stp_bridge.ports[i];
           if (port->state != STP_PORT_STATE_DISABLED) {
               port->state = STP_PORT_STATE_FORWARDING;
           }
       }

       LOG_INFO(LOG_CATEGORY_L2, "STP disabled");
   }

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_set_bridge_priority(uint16_t priority) {
   stp_acquire_lock();

   g_stp_bridge.bridge_id.priority = priority;

   // If new priority makes this bridge the root, update root info
   bridge_id_t old_root_id __attribute__((unused)) = g_stp_bridge.root_id;
   if (compare_bridge_id(g_stp_bridge.bridge_id, g_stp_bridge.root_id) < 0) {
       g_stp_bridge.root_id = g_stp_bridge.bridge_id;
       g_stp_bridge.root_path_cost = 0;
       g_stp_bridge.root_port = PORT_ID_INVALID;

       // Reconfigure topology
       stp_reconfigure_topology();
   }

   LOG_INFO(LOG_CATEGORY_L2, "Bridge priority set to %u", priority);

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_set_port_priority(port_id_t port_id, uint8_t priority) {
   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return ERROR_INVALID_STATE;
   }

   stp_acquire_lock();

   g_stp_bridge.ports[port_id].port_priority = priority;

   // If this is the root port, check if another port should become root
   if (port_id == g_stp_bridge.root_port) {
       stp_reconfigure_topology();
   }

   LOG_INFO(LOG_CATEGORY_L2, "Port %u priority set to %u", port_id, priority);

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_set_port_path_cost(port_id_t port_id, uint32_t path_cost) {
   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return ERROR_INVALID_STATE;
   }

   stp_acquire_lock();

   g_stp_bridge.ports[port_id].path_cost = path_cost;

   // If this is the root port, recalculate root path cost
   if (port_id == g_stp_bridge.root_port) {
       // Recalculate total path cost to root
       g_stp_bridge.root_path_cost = g_stp_bridge.ports[port_id].root_path_cost + path_cost;

       // Send updated BPDUs
       g_stp_bridge.timer_hello = 0;  // Trigger immediate BPDU transmission
   }

   // Check if topology needs to be reconfigured
   stp_reconfigure_topology();

   LOG_INFO(LOG_CATEGORY_L2, "Port %u path cost set to %u", port_id, path_cost);

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_set_port_enabled(port_id_t port_id, bool enable) {
   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return ERROR_INVALID_STATE;
   }

   stp_acquire_lock();

   stp_port_info_t *port = &g_stp_bridge.ports[port_id];

   if (enable && port->state == STP_PORT_STATE_DISABLED) {
       // If this port was disabled, enable it and set to blocking
       port->state = STP_PORT_STATE_BLOCKING;
       port->timer_message_age = 0;
       port->timer_forward_delay = 0;
       port->bpdu_received = false;

       LOG_INFO(LOG_CATEGORY_L2, "STP enabled on port %u", port_id);
   } else if (!enable && port->state != STP_PORT_STATE_DISABLED) {
       // If this port was enabled, disable it
       port->state = STP_PORT_STATE_DISABLED;

       // If this was the root port, we need a new one
       if (port_id == g_stp_bridge.root_port) {
           // Find new root port or become root
           stp_reconfigure_topology();
       }

       LOG_INFO(LOG_CATEGORY_L2, "STP disabled on port %u", port_id);
   }

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_get_port_state(port_id_t port_id, vlan_id_t vlan_id, stp_port_state_t *state) {
    if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    if (vlan_id >= MAX_VLANS) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid VLAN ID %u", vlan_id);
        return STATUS_INVALID_PARAMETER;
    }

    if (!state) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid state pointer");
        return STATUS_INVALID_PARAMETER;
    }

    stp_acquire_lock();
    *state = g_stp_bridge.ports[port_id].vlans[vlan_id].state;
    stp_release_lock();

    return STATUS_SUCCESS;
}



status_t stp_set_port_vlan_state(port_id_t port_id, vlan_id_t vlan_id, stp_port_state_t state) {
    if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    if (vlan_id >= MAX_VLANS) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid VLAN ID %u", vlan_id);
        return STATUS_INVALID_PARAMETER;
    }

    stp_acquire_lock();
    g_stp_bridge.ports[port_id].vlans[vlan_id].state = state;
    stp_release_lock();

    return STATUS_SUCCESS;
}


status_t stp_receive_bpdu(port_id_t port_id, const packet_t *packet) {
   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return ERROR_INVALID_STATE;
   }

   if (!packet) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid packet pointer");
       return ERROR_INVALID_PARAMETER;
   }

   // Check if STP is enabled
   if (!g_stp_bridge.enabled) {
       return ERROR_NOT_INITIALIZED;
   }

   stp_acquire_lock();

   // Process the received BPDU
   status_t result = process_bpdu(port_id, packet);

   stp_release_lock();
   return result;
}

status_t stp_update(uint32_t current_time) {
   // Check if STP is enabled
   if (!g_stp_bridge.enabled) {
       return ERROR_NOT_INITIALIZED;
   }

   stp_acquire_lock();

   // Update port states based on timers
   stp_update_port_states(current_time);

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_is_port_forwarding(port_id_t port_id, bool *forwarding) {
   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return ERROR_INVALID_STATE;
   }

   if (!forwarding) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid forwarding pointer");
       return ERROR_INVALID_PARAMETER;
   }

   stp_acquire_lock();

   *forwarding = (g_stp_bridge.ports[port_id].state == STP_PORT_STATE_FORWARDING);

   stp_release_lock();
   return STATUS_SUCCESS;
}

bool stp_can_forward(port_id_t port_id) {
   if (!g_stp_bridge.enabled) {
       // If STP is disabled, all ports can forward
       return true;
   }

   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return false;
   }

   stp_acquire_lock();

   stp_port_state_t state = g_stp_bridge.ports[port_id].state;

   stp_release_lock();

   // Only forwarding ports can forward data
   return (state == STP_PORT_STATE_FORWARDING);
}

status_t stp_port_link_change(port_id_t port_id, bool link_up) {
   if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
       LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
       return ERROR_INVALID_STATE;
   }

   // Check if STP is enabled
   if (!g_stp_bridge.enabled) {
       return STATUS_SUCCESS;
   }

   stp_acquire_lock();

   stp_port_info_t *port = &g_stp_bridge.ports[port_id];

   if (link_up) {
       // If port was disabled due to link down, restart it
       if (port->state == STP_PORT_STATE_DISABLED) {
           port->state = STP_PORT_STATE_BLOCKING;
           LOG_INFO(LOG_CATEGORY_L2, "Port %u link up, starting in blocking state", port_id);
       }
   } else {
       // Link went down, mark port as disabled
       if (port->state != STP_PORT_STATE_DISABLED) {
           LOG_INFO(LOG_CATEGORY_L2, "Port %u link down, marking as disabled", port_id);
           port->state = STP_PORT_STATE_DISABLED;

           // If this was the root port, need to elect a new one
           if (port_id == g_stp_bridge.root_port) {
               stp_reconfigure_topology();
           }
       }
   }

   stp_release_lock();
   return STATUS_SUCCESS;
}

status_t stp_get_bridge_info(stp_bridge_info_t *info) {
    if (!info) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid info pointer");
        return ERROR_INVALID_PARAMETER;
    }

    stp_acquire_lock();

    info->enabled = g_stp_bridge.enabled;
    info->bridge_id.priority = g_stp_bridge.bridge_id.priority;
    //memcpy(info->bridge_mac, g_stp_bridge.bridge_id.mac_addr, MAC_ADDR_LEN);
    memcpy(info->bridge_id.mac_address.addr, g_stp_bridge.bridge_id.mac_address.addr, MAC_ADDR_LEN);

    //info->root_priority = g_stp_bridge.root_id.priority;
    info->root_id.priority = g_stp_bridge.root_id.priority;
      memcpy(info->root_id.mac_address.addr, g_stp_bridge.root_id.mac_address.addr, MAC_ADDR_LEN);
    //memcpy(info->bridge_mac, g_stp_bridge.bridge_id.mac_addr, MAC_ADDR_LEN);

    info->root_path_cost = g_stp_bridge.root_path_cost;
    info->root_port = g_stp_bridge.root_port;
    info->max_age = g_stp_bridge.max_age;
    info->hello_time = g_stp_bridge.hello_time;
    info->forward_delay = g_stp_bridge.forward_delay;
    info->topology_change = g_stp_bridge.topology_change;
    info->topology_change_time = g_stp_bridge.topology_change_time;

    stp_release_lock();
    return STATUS_SUCCESS;
}

status_t stp_get_port_info(port_id_t port_id, stp_port_info_t *info) {
    if (!port_is_valid(port_id) || port_id >= g_stp_bridge.ports_count) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid port ID %u", port_id);
        return ERROR_INVALID_STATE;
    }
    
    if (!info) {
        LOG_ERROR(LOG_CATEGORY_L2, "Invalid info pointer");
        return ERROR_INVALID_PARAMETER;
    }
    
    stp_acquire_lock();
    
    stp_port_info_t *port = &g_stp_bridge.ports[port_id];
    
    info->port_id = port->port_id;
    info->state = port->state;
    info->port_priority = port->port_priority;
    info->path_cost = port->path_cost;
    info->designated_root = port->designated_root;
    info->root_path_cost = port->root_path_cost;
    info->designated_bridge = port->designated_bridge;
    info->designated_port = port->designated_port;
    info->message_age = port->message_age;
    info->max_age = port->max_age;
    info->hello_time = port->hello_time;
    info->forward_delay = port->forward_delay;
    info->topology_change = port->topology_change;
    info->topology_change_ack = port->topology_change_ack;
    
    stp_release_lock();
    return STATUS_SUCCESS;
}

