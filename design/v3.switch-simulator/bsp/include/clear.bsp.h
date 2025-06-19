#ifndef SDK_ES_SWITCH_SIMULATOR_BSP_INCLUDE_BSP_H
#define SDK_ES_SWITCH_SIMULATOR_BSP_INCLUDE_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define BSP_VERSION_MAJOR           1
#define BSP_VERSION_MINOR           0
#define BSP_VERSION_PATCH           0
#define BSP_VERSION_STRING          "1.0.0"

#define BSP_MAX_PORTS               256
#define BSP_MAX_CALLBACKS           32
#define BSP_MAX_BOARD_NAME_LEN      64
#define BSP_MAX_QOS_QUEUES          8

typedef enum {
  BSP_SUCCESS = 0,
  BSP_ERROR_INVALID_PARAM,
  BSP_ERROR_NULL_POINTER,
  BSP_ERROR_BUFFER_OVERFLOW,
  BSP_ERROR_INVALID_STATE,
  BSP_ERROR_NOT_INITIALIZED,
  BSP_ERROR_INIT_FAILED,
  BSP_ERROR_RESOURCE_UNAVAILABLE,
  BSP_ERROR_IO,
  BSP_ERROR_TIMEOUT,
  BSP_ERROR_NOT_SUPPORTED,
  BSP_ERROR_PORT_NOT_FOUND,
  BSP_ERROR_CONFIG_LOCKED,
  BSP_ERROR_UNKNOWN
} bsp_error_t;

typedef enum {
  BSP_ERROR_INTERNAL       = 100,
  BSP_ERROR_HARDWARE_FAULT,
  BSP_ERROR_DRIVER_ERROR,
  BSP_ERROR_THREAD_SYNC,
  BSP_ERROR_QOS_CONFIG
} bsp_extended_error_t;

typedef enum {
  BSP_RESOURCE_TYPE_BUFFER,
  BSP_RESOURCE_TYPE_DESCRIPTOR,
  BSP_RESOURCE_TYPE_QUEUE,
  BSP_RESOURCE_TYPE_QOS_SCHEDULER
} bsp_resource_type_t;

typedef enum {
  BSP_BOARD_TYPE_GENERIC,
  BSP_BOARD_TYPE_SMALL,
  BSP_BOARD_TYPE_MEDIUM,
  BSP_BOARD_TYPE_LARGE,
  BSP_BOARD_TYPE_DATACENTER,
  BSP_BOARD_TYPE_ENTERPRISE
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

typedef struct {
  uint32_t queue_id;
  uint32_t weight;
  uint32_t max_rate_kbps;
  uint32_t min_rate_kbps;
  bool     strict_priority;
  bool     drop_precedence;
} bsp_qos_queue_t;

typedef struct {
  bsp_qos_queue_t queues[BSP_MAX_QOS_QUEUES];
  uint32_t        queue_count;
  bool            qos_enabled;
  uint32_t        default_queue_id;
} bsp_qos_config_t;

typedef struct {
  bool               link_up;
  bsp_port_speed_t   speed;
  bsp_port_duplex_t  duplex;
  bsp_port_type_t    port_type;
  bool               flow_control_enabled;
  bool               auto_negotiation;
  uint64_t           rx_bytes;
  uint64_t           tx_bytes;
  uint64_t           rx_packets;
  uint64_t           tx_packets;
  uint64_t           rx_errors;
  uint64_t           tx_errors;
  uint64_t           rx_dropped;
  uint64_t           tx_dropped;
  uint32_t           temperature_celsius;
} bsp_port_status_t;

typedef struct {
  bsp_board_type_t board_type;
  uint32_t         num_ports;
  uint32_t         cpu_frequency_mhz;
  uint32_t         memory_size_mb;
  uint32_t         packet_buffer_mb;
  bool             has_layer3_support;
  bool             has_qos_support;
  bool             has_acl_support;
  bool             has_vxlan_support;
  bool             has_sai_support;
  char             board_name[BSP_MAX_BOARD_NAME_LEN];
  char             firmware_version[32];
} bsp_config_t;

typedef struct {
  bool     initialized;
  uint32_t port_count;
  uint32_t active_ports;
  uint32_t failed_ports;
  bool     thread_safe_mode;

  // BSP version information:
  uint32_t version_major;
  uint32_t version_minor;
  uint32_t version_patch;

  // System resources:
  uint32_t memory_used_mb;
  uint32_t memory_free_mb;
  uint64_t uptime_seconds;
} bsp_status_t;

typedef struct {
  pthread_mutex_t config_mutex;
  pthread_mutex_t port_mutex[BSP_MAX_PORTS];
  pthread_mutex_t resource_mutex;
  bool            thread_safe_mode;
  uint32_t        active_threads;
} bsp_threading_t;

typedef void* bsp_resource_handle_t;

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

bsp_error_t bsp_init(const bsp_config_t* config);
bsp_error_t bsp_deinit(void);
bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config);
bsp_error_t bsp_get_config(bsp_config_t* config);
bsp_error_t bsp_set_config(const bsp_config_t* config);
bsp_error_t bsp_reset(bool hard_reset);
bsp_error_t bsp_set_thread_safe_mode(bool enable);
bsp_error_t bsp_port_init(uint32_t port_id, bsp_port_speed_t speed, bsp_port_duplex_t duplex);
bsp_error_t bsp_port_init_advanced(uint32_t port_id,
    bsp_port_speed_t speed,
    bsp_port_duplex_t duplex,
    bsp_port_type_t port_type,
    bool auto_neg);
bsp_error_t bsp_port_get_status(uint32_t port_id, bsp_port_status_t* status);
bsp_error_t bsp_port_set_enabled(uint32_t port_id, bool enable);
bsp_error_t bsp_port_set_qos_config(uint32_t port_id, const bsp_qos_config_t* qos_config);
bsp_error_t bsp_port_get_qos_config(uint32_t port_id, bsp_qos_config_t* qos_config);
bsp_error_t bsp_port_set_flow_control(uint32_t port_id, bool enable);
bsp_error_t bsp_port_clear_stats(uint32_t port_id);
bsp_error_t bsp_port_register_callback(uint32_t port_id,
    void (*callback)(uint32_t port_id,
      bsp_port_status_t status,
      void* user_data),
    void* user_data);
bsp_error_t bsp_port_unregister_callback(uint32_t port_id);
bsp_error_t bsp_allocate_resource(bsp_resource_type_t resource_type, uint32_t size, bsp_resource_handle_t* handle);
bsp_error_t bsp_free_resource(bsp_resource_handle_t handle);
uint64_t bsp_get_timestamp_us(void);
uint64_t bsp_get_timestamp_ns(void);
bool bsp_is_initialized(void);
const bsp_config_t* bsp_get_current_config(void);
const char* bsp_get_version(void);
bsp_error_t bsp_get_status(bsp_status_t* status);
bsp_error_t bsp_get_memory_info(uint32_t* total_mb, uint32_t* used_mb, uint32_t* free_mb);
bsp_error_t bsp_run_diagnostics(uint32_t* test_results);
bsp_error_t bsp_init_config(bsp_config_t* config, bsp_board_type_t board_type);
bsp_error_t bsp_set_board_name(bsp_config_t* config, const char* name);

#endif /* SDK_ES_SWITCH_SIMULATOR_BSP_INCLUDE_BSP_H */

