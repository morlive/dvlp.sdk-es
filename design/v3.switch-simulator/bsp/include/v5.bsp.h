/**
* @file bsp.h
* @brief Board Support Package main interface header
*
* This file contains public declarations for BSP (Board Support Package),
* providing hardware abstraction for the switch simulator and enterprise
* switching solutions.
*/

#ifndef BSP_H
#define BSP_H

// -----------------------------------------------------------------------------
// 0. Standard headers inclusion
// -----------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// -----------------------------------------------------------------------------
// 1. BSP Version (major, minor, patch and string representation)
// -----------------------------------------------------------------------------
#define BSP_VERSION_MAJOR   1
#define BSP_VERSION_MINOR   0
#define BSP_VERSION_PATCH   0
#define BSP_VERSION_STRING  "1.0.0"

// -----------------------------------------------------------------------------
// 2. Maximum values constants
// -----------------------------------------------------------------------------
#define BSP_MAX_PORTS           256
#define BSP_MAX_CALLBACKS       32
#define BSP_MAX_BOARD_NAME_LEN  64
#define BSP_MAX_QOS_QUEUES      8

// -----------------------------------------------------------------------------
// 3. Basic BSP error codes (range 0..99)
// -----------------------------------------------------------------------------
typedef enum {
   BSP_SUCCESS = 0,
   BSP_ERROR_INVALID_PARAM,
   BSP_ERROR_NULL_POINTER,
   BSP_ERROR_BUFFER_OVERFLOW,
   BSP_ERROR_INVALID_STATE,
   BSP_ERROR_NOT_INITIALIZED,
   BSP_ERROR_RESOURCE_UNAVAILABLE,
   BSP_ERROR_IO,
   BSP_ERROR_TIMEOUT,
   BSP_ERROR_NOT_SUPPORTED,
   BSP_ERROR_PORT_NOT_FOUND,
   BSP_ERROR_CONFIG_LOCKED,
   BSP_ERROR_UNKNOWN
} bsp_error_t;

// -----------------------------------------------------------------------------
// 4. Extended error codes (vendor-specific, driver, hardware) (100..199)
// -----------------------------------------------------------------------------
typedef enum {
   BSP_ERROR_INTERNAL       = 100,  // used in bsp_get_config() for example
   BSP_ERROR_HARDWARE_FAULT,        // hardware or emulation failure
   BSP_ERROR_DRIVER_ERROR,          // low-level driver error
   BSP_ERROR_THREAD_SYNC,           // threading synchronization error
   BSP_ERROR_QOS_CONFIG             // QoS configuration error
} bsp_extended_error_t;

// -----------------------------------------------------------------------------
// 5. Resource types (for bsp_allocate_resource/bsp_free_resource)
// -----------------------------------------------------------------------------
typedef enum {
   BSP_RESOURCE_TYPE_BUFFER,
   BSP_RESOURCE_TYPE_DESCRIPTOR,
   BSP_RESOURCE_TYPE_QUEUE,
   BSP_RESOURCE_TYPE_QOS_SCHEDULER
} bsp_resource_type_t;

// -----------------------------------------------------------------------------
// 6. Board types and port speed/duplex/type definitions
// -----------------------------------------------------------------------------
typedef enum {
   BSP_BOARD_TYPE_GENERIC,
   BSP_BOARD_TYPE_SMALL,      // small board (~8 ports)
   BSP_BOARD_TYPE_MEDIUM,     // medium board (~24 ports)
   BSP_BOARD_TYPE_LARGE,      // large board (~48 ports)
   BSP_BOARD_TYPE_DATACENTER, // datacenter board, high port density
   BSP_BOARD_TYPE_ENTERPRISE  // enterprise switching board
} bsp_board_type_t;

typedef enum {
   BSP_PORT_SPEED_10M   = 10,
   BSP_PORT_SPEED_100M  = 100,
   BSP_PORT_SPEED_1G    = 1000,
   BSP_PORT_SPEED_10G   = 10000,
   BSP_PORT_SPEED_25G   = 25000,
   BSP_PORT_SPEED_40G   = 40000,
   BSP_PORT_SPEED_100G  = 100000,
   BSP_PORT_SPEED_200G  = 200000,
   BSP_PORT_SPEED_400G  = 400000,
   BSP_PORT_SPEED_800G  = 800000
} bsp_port_speed_t;

typedef enum {
   BSP_PORT_DUPLEX_HALF,
   BSP_PORT_DUPLEX_FULL
} bsp_port_duplex_t;

typedef enum {
   BSP_PORT_TYPE_COPPER,
   BSP_PORT_TYPE_FIBER,
   BSP_PORT_TYPE_SFP,
   BSP_PORT_TYPE_SFP_PLUS,
   BSP_PORT_TYPE_QSFP,
   BSP_PORT_TYPE_QSFP_PLUS,
   BSP_PORT_TYPE_QSFP_DD,
   BSP_PORT_TYPE_OSFP
} bsp_port_type_t;

// -----------------------------------------------------------------------------
// 7. QoS queue configuration structure
// -----------------------------------------------------------------------------
typedef struct {
   uint32_t queue_id;              // queue identifier (0-7)
   uint32_t weight;                // queue weight for WRR scheduling
   uint32_t max_rate_kbps;         // maximum rate in kbps (0 = unlimited)
   uint32_t min_rate_kbps;         // minimum guaranteed rate in kbps
   bool     strict_priority;       // true for strict priority scheduling
   bool     drop_precedence;       // drop precedence for WRED
} bsp_qos_queue_t;

typedef struct {
   bsp_qos_queue_t queues[BSP_MAX_QOS_QUEUES];
   uint32_t        queue_count;
   bool            qos_enabled;
   uint32_t        default_queue_id;
} bsp_qos_config_t;

// -----------------------------------------------------------------------------
// 8. Port status and board configuration structures
// -----------------------------------------------------------------------------
typedef struct {
   bool               link_up;              // true if link is up
   bsp_port_speed_t   speed;                // port speed
   bsp_port_duplex_t  duplex;               // duplex mode
   bsp_port_type_t    port_type;            // physical port type
   bool               flow_control_enabled; // flow control enabled
   bool               auto_negotiation;     // auto-negotiation enabled
   uint64_t           rx_bytes;             // received bytes
   uint64_t           tx_bytes;             // transmitted bytes
   uint64_t           rx_packets;           // received packets
   uint64_t           tx_packets;           // transmitted packets
   uint64_t           rx_errors;            // receive errors
   uint64_t           tx_errors;            // transmit errors
   uint64_t           rx_dropped;           // received packets dropped
   uint64_t           tx_dropped;           // transmitted packets dropped
   uint32_t           temperature_celsius;  // port temperature (if available)
} bsp_port_status_t;

typedef struct {
   bsp_board_type_t board_type;        // board type (enum above)
   uint32_t         num_ports;         // total number of ports
   uint32_t         cpu_frequency_mhz; // CPU frequency (e.g., simulated)
   uint32_t         memory_size_mb;    // memory size (MB)
   uint32_t         packet_buffer_mb;  // packet buffer memory (MB)
   bool             has_layer3_support;// L3 support (true/false)
   bool             has_qos_support;   // QoS support
   bool             has_acl_support;   // ACL support
   bool             has_vxlan_support; // VXLAN support
   bool             has_sai_support;   // SAI compatibility
   char             board_name[BSP_MAX_BOARD_NAME_LEN]; // board name with size limit
   char             firmware_version[32]; // firmware version string
} bsp_config_t;

/**
* @brief Structure for returning general BSP status
*
* Used in bsp_get_status(). Contains flags, port count, number of active ports,
* and BSP version information (major/minor/patch).
*/
typedef struct {
   bool     initialized;    // true if bsp_init() has been executed
   uint32_t port_count;     // current number of ports
   uint32_t active_ports;   // number of ports in UP state
   uint32_t failed_ports;   // number of ports in FAILED state
   bool     thread_safe_mode; // thread-safe mode enabled
   // BSP version information:
   uint32_t version_major;
   uint32_t version_minor;
   uint32_t version_patch;
   // System resources:
   uint32_t memory_used_mb;    // used memory in MB
   uint32_t memory_free_mb;    // free memory in MB
   uint64_t uptime_seconds;    // BSP uptime in seconds
} bsp_status_t;

// -----------------------------------------------------------------------------
// 9. Threading support structure
// -----------------------------------------------------------------------------
typedef struct {
   pthread_mutex_t config_mutex;                    // configuration protection
   pthread_mutex_t port_mutex[BSP_MAX_PORTS];      // per-port protection
   pthread_mutex_t resource_mutex;                 // resource allocation protection
   bool            thread_safe_mode;               // thread-safe mode flag
   uint32_t        active_threads;                 // number of active threads
} bsp_threading_t;

// -----------------------------------------------------------------------------
// 10. Resource handle type (opaque pointer)
// -----------------------------------------------------------------------------
typedef void* bsp_resource_handle_t;

// -----------------------------------------------------------------------------
// 11. Parameter validation macros
// -----------------------------------------------------------------------------
#define BSP_VALIDATE_PORT_ID(port_id) \
   ((port_id) < bsp_get_current_config()->num_ports)

#define BSP_IS_VALID_SPEED(speed) \
   (((speed) >= BSP_PORT_SPEED_10M) && ((speed) <= BSP_PORT_SPEED_800G))

#define BSP_IS_VALID_BOARD_TYPE(type) \
   (((type) >= BSP_BOARD_TYPE_GENERIC) && ((type) <= BSP_BOARD_TYPE_ENTERPRISE))

#define BSP_IS_VALID_PORT_TYPE(type) \
   (((type) >= BSP_PORT_TYPE_COPPER) && ((type) <= BSP_PORT_TYPE_OSFP))

#define BSP_IS_VALID_QOS_QUEUE(queue_id) \
   ((queue_id) < BSP_MAX_QOS_QUEUES)

// -----------------------------------------------------------------------------
// 12. BSP-API function prototypes
// -----------------------------------------------------------------------------

/**
* @brief Initialize the Board Support Package
*
* This function initializes the BSP with the provided configuration.
* Must be called before any other BSP functions.
*
* @param[in] config Pointer to board configuration structure
* @return BSP_SUCCESS or error code from bsp_error_t
*/
bsp_error_t bsp_init(const bsp_config_t* config);

/**
* @brief Deinitialize the Board Support Package and release all resources
*
* This function cleans up all allocated resources and resets the BSP state.
*
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_deinit(void);

/**
* @brief Get default configuration for specified board type
*
* @param[in]  board_type Board type (enum bsp_board_type_t)
* @param[out] config     Configuration structure to be filled
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config);

/**
* @brief Get current BSP configuration (copies data to provided buffer)
*
* @param[out] config Pointer to bsp_config_t where current configuration will be written
* @return BSP_SUCCESS or error code (e.g., BSP_ERROR_NOT_INITIALIZED)
*/
bsp_error_t bsp_get_config(bsp_config_t* config);

/**
* @brief Set (modify) BSP configuration on-the-fly
*
* @param[in] config Pointer to new configuration
* @return BSP_SUCCESS or error code (e.g., BSP_ERROR_CONFIG_LOCKED)
*/
bsp_error_t bsp_set_config(const bsp_config_t* config);

/**
* @brief Reset BSP to default state using current configuration
*
* @param[in] hard_reset true = full reset, false = soft reset
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_reset(bool hard_reset);

/**
* @brief Enable or disable thread-safe mode
*
* When enabled, all BSP operations will be protected by mutexes.
* Should be called before bsp_init() for optimal performance.
*
* @param[in] enable true = enable thread-safe mode, false = disable
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_set_thread_safe_mode(bool enable);

/**
* @brief Initialize a specific port
*
* @param[in] port_id Identifier of the port (0 ... num_ports-1)
* @param[in] speed   Port speed (enum bsp_port_speed_t)
* @param[in] duplex  Duplex mode (enum bsp_port_duplex_t)
* @return BSP_SUCCESS or error code (e.g., BSP_ERROR_PORT_NOT_FOUND)
*/
bsp_error_t bsp_port_init(uint32_t port_id, bsp_port_speed_t speed, bsp_port_duplex_t duplex);

/**
* @brief Initialize a specific port with advanced configuration
*
* @param[in] port_id   Port identifier
* @param[in] speed     Port speed
* @param[in] duplex    Duplex mode
* @param[in] port_type Physical port type
* @param[in] auto_neg  Enable auto-negotiation
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_init_advanced(uint32_t port_id, 
                                 bsp_port_speed_t speed, 
                                 bsp_port_duplex_t duplex,
                                 bsp_port_type_t port_type,
                                 bool auto_neg);

/**
* @brief Get the current status of a port
*
* @param[in]  port_id Port identifier
* @param[out] status  Pointer to bsp_port_status_t
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_get_status(uint32_t port_id, bsp_port_status_t* status);

/**
* @brief Enable or disable a given port
*
* @param[in] port_id Port identifier
* @param[in] enable  true = enable port, false = disable
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_set_enabled(uint32_t port_id, bool enable);

/**
* @brief Set QoS configuration for a port
*
* @param[in] port_id    Port identifier
* @param[in] qos_config Pointer to QoS configuration structure
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_set_qos_config(uint32_t port_id, const bsp_qos_config_t* qos_config);

/**
* @brief Get QoS configuration for a port
*
* @param[in]  port_id    Port identifier
* @param[out] qos_config Pointer to QoS configuration structure to be filled
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_get_qos_config(uint32_t port_id, bsp_qos_config_t* qos_config);

/**
* @brief Set flow control configuration for a port
*
* @param[in] port_id Port identifier
* @param[in] enable  Enable/disable flow control
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_set_flow_control(uint32_t port_id, bool enable);

/**
* @brief Clear port statistics counters
*
* @param[in] port_id Port identifier
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_clear_stats(uint32_t port_id);

/**
* @brief Register a callback for port status change notifications
*
* @param[in] port_id   Port identifier
* @param[in] callback  Function: void callback(uint32_t port_id, bsp_port_status_t status, void* user_data)
* @param[in] user_data User data passed to callback
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_register_callback(uint32_t port_id,
                                      void (*callback)(uint32_t port_id,
                                                       bsp_port_status_t status,
                                                       void* user_data),
                                      void* user_data);

/**
* @brief Unregister port status change callback
*
* @param[in] port_id Port identifier
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_port_unregister_callback(uint32_t port_id);

/**
* @brief Allocate a generic resource (e.g., buffer, descriptor) in BSP
*
* @param[in]  resource_type Resource type (enum bsp_resource_type_t)
* @param[in]  size          Size in bytes
* @param[out] handle        Output handle (opaque pointer)
* @return BSP_SUCCESS or error code (e.g., BSP_ERROR_RESOURCE_UNAVAILABLE)
*/
bsp_error_t bsp_allocate_resource(bsp_resource_type_t resource_type, uint32_t size, bsp_resource_handle_t* handle);

/**
* @brief Free previously allocated resource
*
* @param[in] handle Handle obtained from bsp_allocate_resource()
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_free_resource(bsp_resource_handle_t handle);

/**
* @brief Get current timestamp in microseconds
*
* @return Timestamp in microseconds
*/
uint64_t bsp_get_timestamp_us(void);

/**
* @brief Get current timestamp in nanoseconds (high precision)
*
* @return Timestamp in nanoseconds
*/
uint64_t bsp_get_timestamp_ns(void);

/**
* @brief Check if BSP is already initialized
*
* @return true if BSP is initialized, false otherwise
*/
bool bsp_is_initialized(void);

/**
* @brief Helper function for validation macros:
*        returns pointer to current BSP configuration
*
* @return Pointer to static bsp_config_t (if BSP is initialized), NULL otherwise
*/
const bsp_config_t* bsp_get_current_config(void);

/**
* @brief Get BSP version as string
*
* @return String in format "MAJOR.MINOR.PATCH"
*/
const char* bsp_get_version(void);

/**
* @brief Get detailed status of BSP
*
* Returns comprehensive BSP status including initialization state,
* port counts, threading mode, memory usage, and uptime.
*
* @param[out] status Pointer to bsp_status_t to be filled
* @return BSP_SUCCESS or error code (e.g., BSP_ERROR_INVALID_PARAM)
*/
bsp_error_t bsp_get_status(bsp_status_t* status);

/**
* @brief Get system memory information
*
* @param[out] total_mb  Total system memory in MB
* @param[out] used_mb   Used memory in MB
* @param[out] free_mb   Free memory in MB
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_get_memory_info(uint32_t* total_mb, uint32_t* used_mb, uint32_t* free_mb);

/**
* @brief Perform BSP self-test and diagnostics
*
* Runs comprehensive self-test including port functionality,
* memory tests, and internal consistency checks.
*
* @param[out] test_results Bitmask of test results (0 = all passed)
* @return BSP_SUCCESS or error code
*/
bsp_error_t bsp_run_diagnostics(uint32_t* test_results);

// -----------------------------------------------------------------------------
// 13. Integration notes for HAL and SAI layers
// -----------------------------------------------------------------------------

/**
* Integration Notes:
* 
* HAL Integration:
* - This BSP layer provides hardware abstraction for HAL components
* - HAL port.h should use bsp_port_* functions for hardware operations
* - HAL packet.h can use BSP resource allocation for buffer management
* 
* SAI Compatibility:
* - BSP provides foundation for SAI adapter implementation
* - Port configurations map directly to SAI port attributes
* - QoS structures align with SAI QoS model
* - Statistics counters follow SAI counter naming conventions
* 
* Python Bindings:
* - Consider creating Python wrappers for BSP API
* - Use ctypes or SWIG for automatic binding generation
* - Expose port status and statistics for monitoring tools
*/

#endif /* BSP_H */