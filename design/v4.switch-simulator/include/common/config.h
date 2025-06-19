#ifndef SWITCH_SIM_CONFIG_H
#define SWITCH_SIM_CONFIG_H

/**
 * @brief Количество портов по умолчанию для симулятора
 */
#define CONFIG_DEFAULT_PORT_COUNT 128


#endif /* SWITCH_SIM_CONFIG_H */


/**
 * @file config.h
 * @brief System configuration parameters and limits for switch simulator
 *
 * This file contains compile-time configuration constants that define
 * system-wide limits, default values, and behavioral parameters for the
 * switch simulator. All values can be overridden at compile time using
 * preprocessor definitions.
 *
 * @note Configuration values should be tuned based on target hardware
 *       capabilities and expected traffic patterns.
 */
#ifndef SWITCH_SIM_CONFIG_H
#define SWITCH_SIM_CONFIG_H

#include <stdint.h>
#include <stdbool.h>


/*===========================================================================*/
/* SYSTEM CONFIGURATION                                                      */
/*===========================================================================*/

/**
 * @brief Maximum number of physical ports supported by the simulator
 *
 * This defines the upper limit for port identifiers and affects memory
 * allocation for port-related data structures.
 */
#ifndef CONFIG_MAX_PORTS
#define CONFIG_MAX_PORTS                    256
#endif

/**
 * @brief Default number of ports to initialize
 *
 * The simulator will initialize this many ports by default unless
 * explicitly configured otherwise at runtime.
 */
#ifndef CONFIG_DEFAULT_PORT_COUNT
#define CONFIG_DEFAULT_PORT_COUNT           128
#endif

/**
 * @brief Maximum number of VLANs supported
 *
 * Valid VLAN IDs range from 1 to this value. VLAN 0 is reserved
 * for untagged traffic.
 */
#ifndef CONFIG_MAX_VLANS
#define CONFIG_MAX_VLANS                    4094
#endif

/**
 * @brief Default VLAN ID for untagged traffic
 */
#ifndef CONFIG_DEFAULT_VLAN_ID
#define CONFIG_DEFAULT_VLAN_ID              1
#endif


/*===========================================================================*/
/* MEMORY AND PERFORMANCE CONFIGURATION                                      */
/*===========================================================================*/

/**
 * @brief Maximum number of MAC address table entries
 *
 * This determines the size of the hash table used for MAC address learning.
 * Higher values provide better scalability but consume more memory.
 */
#ifndef CONFIG_MAX_MAC_TABLE_ENTRIES
#define CONFIG_MAX_MAC_TABLE_ENTRIES        65536
#endif

/**
 * @brief Default MAC address aging time in seconds
 *
 * MAC addresses learned dynamically will be removed if not seen
 * for this duration. Set to 0 to disable aging.
 */
#ifndef CONFIG_DEFAULT_MAC_AGING_TIME
#define CONFIG_DEFAULT_MAC_AGING_TIME       300
#endif

/**
 * @brief Maximum number of routing table entries
 *
 * Defines the capacity of the L3 routing table for IPv4 and IPv6 routes.
 */
#ifndef CONFIG_MAX_ROUTING_ENTRIES
#define CONFIG_MAX_ROUTING_ENTRIES          16384
#endif

/**
 * @brief Maximum number of ARP table entries
 *
 * Controls the size of the ARP resolution table for IPv4 addresses.
 */
#ifndef CONFIG_MAX_ARP_ENTRIES
#define CONFIG_MAX_ARP_ENTRIES              8192
#endif

/**
 * @brief Default ARP entry aging time in seconds
 */
#ifndef CONFIG_DEFAULT_ARP_AGING_TIME
#define CONFIG_DEFAULT_ARP_AGING_TIME       1200    /* 20 minutes */
#endif


/*===========================================================================*/
/* PACKET PROCESSING CONFIGURATION                                           */
/*===========================================================================*/

/**
 * @brief Maximum transmission unit (MTU) in bytes
 *
 * Largest packet size that can be transmitted without fragmentation.
 */
#ifndef CONFIG_MAX_MTU
#define CONFIG_MAX_MTU                      9216    /* Jumbo frame support */
#endif

/**
 * @brief Default MTU for new interfaces
 */
#ifndef CONFIG_DEFAULT_MTU
#define CONFIG_DEFAULT_MTU                  1500    /* Standard Ethernet MTU */
#endif

/**
 * @brief Maximum packet buffer size in bytes
 *
 * Internal buffer size for packet processing. Should be larger than
 * MAX_MTU to accommodate headers and metadata.
 */
#ifndef CONFIG_MAX_PACKET_SIZE
#define CONFIG_MAX_PACKET_SIZE              (CONFIG_MAX_MTU + 256)
#endif

/**
 * @brief Number of packet buffers to pre-allocate
 *
 * Higher values improve performance under high load but consume more memory.
 */
#ifndef CONFIG_PACKET_BUFFER_POOL_SIZE
#define CONFIG_PACKET_BUFFER_POOL_SIZE      4096
#endif

/**
 * @brief Maximum number of fragments per IP packet
 *
 * Limits memory usage for packet reassembly operations.
 */
#ifndef CONFIG_MAX_IP_FRAGMENTS
#define CONFIG_MAX_IP_FRAGMENTS             64
#endif

/**
 * @brief IP fragment reassembly timeout in seconds
 */
#ifndef CONFIG_IP_FRAGMENT_TIMEOUT
#define CONFIG_IP_FRAGMENT_TIMEOUT          30
#endif


/*===========================================================================*/
/* THREADING AND CONCURRENCY CONFIGURATION                                   */
/*===========================================================================*/

/**
 * @brief Number of worker threads for packet processing
 *
 * Should typically match the number of CPU cores for optimal performance.
 * Set to 0 to auto-detect based on system capabilities.
 */
#ifndef CONFIG_WORKER_THREADS
#define CONFIG_WORKER_THREADS               0       /* Auto-detect */
#endif

/**
 * @brief Maximum queue depth for inter-thread communication
 */
#ifndef CONFIG_MAX_QUEUE_DEPTH
#define CONFIG_MAX_QUEUE_DEPTH              1024
#endif

/**
 * @brief Lock-free ring buffer size (must be power of 2)
 */
#ifndef CONFIG_RING_BUFFER_SIZE
#define CONFIG_RING_BUFFER_SIZE             2048
#endif


/*===========================================================================*/
/* LOGGING AND DEBUGGING CONFIGURATION                                       */
/*===========================================================================*/

/**
 * @brief Default log level for system components
 * 
 * Valid values: 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG, 4=TRACE
 */
#ifndef CONFIG_DEFAULT_LOG_LEVEL
#define CONFIG_DEFAULT_LOG_LEVEL            2       /* INFO */
#endif

/**
 * @brief Maximum length of log messages in characters
 */
#ifndef CONFIG_MAX_LOG_MESSAGE_LENGTH
#define CONFIG_MAX_LOG_MESSAGE_LENGTH       512
#endif

/**
 * @brief Size of circular log buffer (number of messages)
 */
#ifndef CONFIG_LOG_BUFFER_SIZE
#define CONFIG_LOG_BUFFER_SIZE              10000
#endif

/**
 * @brief Enable/disable runtime statistics collection
 * 
 * When enabled, the system collects detailed performance metrics.
 * Disable in production for better performance.
 */
#ifndef CONFIG_ENABLE_STATISTICS
#define CONFIG_ENABLE_STATISTICS            true
#endif

/**
 * @brief Statistics collection interval in milliseconds
 */
#ifndef CONFIG_STATS_COLLECTION_INTERVAL_MS
#define CONFIG_STATS_COLLECTION_INTERVAL_MS 1000
#endif


/*===========================================================================*/
/* SIMULATION-SPECIFIC CONFIGURATION                                         */
/*===========================================================================*/

/**
 * @brief Enable hardware simulation mode
 * 
 * When true, the system simulates hardware behavior including
 * delays, resource limitations, and error conditions.
 */
#ifndef CONFIG_ENABLE_HW_SIMULATION
#define CONFIG_ENABLE_HW_SIMULATION         true
#endif

/**
 * @brief Simulated packet processing delay in microseconds
 * 
 * Artificial delay added to packet processing to simulate
 * hardware pipeline latency.
 */
#ifndef CONFIG_SIM_PACKET_DELAY_US
#define CONFIG_SIM_PACKET_DELAY_US          10
#endif

/**
 * @brief Simulated memory access delay in nanoseconds
 */
#ifndef CONFIG_SIM_MEMORY_DELAY_NS
#define CONFIG_SIM_MEMORY_DELAY_NS          100
#endif

/**
 * @brief Enable random packet drop simulation
 * 
 * When enabled, packets are randomly dropped to simulate
 * network conditions and hardware limitations.
 */
#ifndef CONFIG_ENABLE_PACKET_DROP_SIM
#define CONFIG_ENABLE_PACKET_DROP_SIM       false
#endif

/**
 * @brief Packet drop probability (0.0 to 1.0)
 * 
 * Probability of dropping a packet when packet drop simulation
 * is enabled. 0.001 = 0.1% drop rate.
 */
#ifndef CONFIG_PACKET_DROP_PROBABILITY
#define CONFIG_PACKET_DROP_PROBABILITY      0.001
#endif


/*===========================================================================*/
/* PROTOCOL-SPECIFIC CONFIGURATION                                           */
/*===========================================================================*/

/**
 * @brief Enable IPv6 support
 * 
 * When disabled, IPv6 packets are dropped and IPv6-related
 * features are not available.
 */
#ifndef CONFIG_ENABLE_IPV6
#define CONFIG_ENABLE_IPV6                  true
#endif

/**
 * @brief Enable OSPF routing protocol support
 */
#ifndef CONFIG_ENABLE_OSPF
#define CONFIG_ENABLE_OSPF                  true
#endif

/**
 * @brief Enable RIP routing protocol support
 */
#ifndef CONFIG_ENABLE_RIP
#define CONFIG_ENABLE_RIP                   true
#endif

/**
 * @brief Enable Spanning Tree Protocol (STP) support
 */
#ifndef CONFIG_ENABLE_STP
#define CONFIG_ENABLE_STP                   true
#endif

/**
 * @brief Enable Quality of Service (QoS) features
 */
#ifndef CONFIG_ENABLE_QOS
#define CONFIG_ENABLE_QOS                   true
#endif


/*===========================================================================*/
/* VALIDATION MACROS                                                         */
/*===========================================================================*/

/* Compile-time validation of configuration parameters */
#if CONFIG_MAX_PORTS > 65535
#error "CONFIG_MAX_PORTS cannot exceed 65535"
#endif

#if CONFIG_DEFAULT_PORT_COUNT > CONFIG_MAX_PORTS
#error "CONFIG_DEFAULT_PORT_COUNT cannot exceed CONFIG_MAX_PORTS"
#endif

#if CONFIG_MAX_VLANS > 4094
#error "CONFIG_MAX_VLANS cannot exceed 4094 (IEEE 802.1Q limit)"
#endif

#if CONFIG_DEFAULT_VLAN_ID > CONFIG_MAX_VLANS
#error "CONFIG_DEFAULT_VLAN_ID cannot exceed CONFIG_MAX_VLANS"
#endif

#if (CONFIG_RING_BUFFER_SIZE & (CONFIG_RING_BUFFER_SIZE - 1)) != 0
#error "CONFIG_RING_BUFFER_SIZE must be a power of 2"
#endif


/*===========================================================================*/
/* DERIVED CONFIGURATION VALUES                                              */
/*===========================================================================*/

/**
 * @brief Hash table size for MAC address table
 * 
 * Calculated as next power of 2 greater than or equal to
 * CONFIG_MAX_MAC_TABLE_ENTRIES for optimal hash distribution.
 */
#define CONFIG_MAC_TABLE_HASH_SIZE          (1 << (32 - __builtin_clz(CONFIG_MAX_MAC_TABLE_ENTRIES - 1)))

/**
 * @brief Maximum number of concurrent packet processing contexts
 * 
 * Derived from number of worker threads and queue depth.
 */
#define CONFIG_MAX_PROCESSING_CONTEXTS      (CONFIG_WORKER_THREADS * 4)

/**
 * @brief Total memory allocation for packet buffers in bytes
 */
#define CONFIG_TOTAL_PACKET_BUFFER_MEMORY   (CONFIG_PACKET_BUFFER_POOL_SIZE * CONFIG_MAX_PACKET_SIZE)


/*===========================================================================*/
/* FEATURE FLAGS                                                             */
/*===========================================================================*/

/**
 * @brief Compile-time feature enable/disable flags
 * 
 * These flags can be used with #ifdef to conditionally compile
 * features and reduce binary size.
 */
#define FEATURE_MAC_LEARNING                1
#define FEATURE_VLAN_SUPPORT               1
#define FEATURE_STP_SUPPORT                CONFIG_ENABLE_STP
#define FEATURE_L3_ROUTING                 1
#define FEATURE_IPV6_SUPPORT               CONFIG_ENABLE_IPV6
#define FEATURE_QOS_SUPPORT                CONFIG_ENABLE_QOS
#define FEATURE_STATISTICS                 CONFIG_ENABLE_STATISTICS
#define FEATURE_HARDWARE_SIMULATION        CONFIG_ENABLE_HW_SIMULATION




#endif /* SWITCH_SIM_CONFIG_H */

