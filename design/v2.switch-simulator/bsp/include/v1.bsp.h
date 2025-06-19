/**
 * @file bsp.h
 * @brief Board Support Package main interface header
 *
 * This file defines the interfaces for the BSP (Board Support Package)
 * which provides hardware abstraction for the switch simulator.
 */
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BSP_SUCCESS = 0,
    BSP_ERROR_INVALID_PARAM,
    BSP_ERROR_NOT_INITIALIZED,
    BSP_ERROR_RESOURCE_UNAVAILABLE,
    BSP_ERROR_IO,
    BSP_ERROR_TIMEOUT,
    BSP_ERROR_NOT_SUPPORTED,
    BSP_ERROR_UNKNOWN
} bsp_error_t;

typedef enum {
    BSP_BOARD_TYPE_GENERIC,
    BSP_BOARD_TYPE_SMALL,      // Small switch (e.g., 8 ports)
    BSP_BOARD_TYPE_MEDIUM,     // Medium switch (e.g., 24 ports)
    BSP_BOARD_TYPE_LARGE,      // Large switch (e.g., 48 ports)
    BSP_BOARD_TYPE_DATACENTER  // High-density datacenter switch
} bsp_board_type_t;

typedef enum {
    BSP_PORT_SPEED_10M = 10,
    BSP_PORT_SPEED_100M = 100,
    BSP_PORT_SPEED_1G = 1000,
    BSP_PORT_SPEED_10G = 10000,
    BSP_PORT_SPEED_25G = 25000,
    BSP_PORT_SPEED_40G = 40000,
    BSP_PORT_SPEED_100G = 100000
} bsp_port_speed_t;

typedef enum {
    BSP_PORT_DUPLEX_HALF,
    BSP_PORT_DUPLEX_FULL
} bsp_port_duplex_t;

typedef struct {
    bool link_up;
    bsp_port_speed_t speed;
    bsp_port_duplex_t duplex;
    bool flow_control_enabled;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_errors;
    uint64_t tx_errors;
} bsp_port_status_t;

typedef struct {
    bsp_board_type_t board_type;
    uint32_t num_ports;
    uint32_t cpu_frequency_mhz;
    uint32_t memory_size_mb;
    bool has_layer3_support;
    bool has_qos_support;
    bool has_acl_support;
    const char* board_name;
} bsp_config_t;

typedef void* bsp_resource_handle_t;


// VALIDATION MACROS - размещаем здесь, после типов но до функций
#define BSP_VALIDATE_PORT_ID(port_id) \
    ((port_id) < bsp_get_current_config()->num_ports)

#define BSP_IS_VALID_SPEED(speed) \
    ((speed) >= BSP_PORT_SPEED_10M && (speed) <= BSP_PORT_SPEED_100G)

#define BSP_IS_VALID_BOARD_TYPE(type) \
    ((type) >= BSP_BOARD_TYPE_GENERIC && (type) <= BSP_BOARD_TYPE_DATACENTER)


bsp_error_t bsp_init(const bsp_config_t* config);

bsp_error_t bsp_deinit(void);

bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config);

bsp_error_t bsp_get_config(bsp_config_t* config);

/**
* @brief Set the board configuration
*/
bsp_error_t bsp_set_config(const bsp_config_t* config);

bsp_error_t bsp_reset(bool hard_reset);

bsp_error_t bsp_port_init(uint32_t port_id, bsp_port_speed_t speed, bsp_port_duplex_t duplex);

bsp_error_t bsp_port_get_status(uint32_t port_id, bsp_port_status_t* status);

bsp_error_t bsp_port_set_enabled(uint32_t port_id, bool enable);

bsp_error_t bsp_port_register_callback(uint32_t port_id, 
                                      void (*callback)(uint32_t port_id, bsp_port_status_t status, void* user_data),
                                      void* user_data);

bsp_error_t bsp_allocate_resource(uint32_t resource_type, uint32_t size, bsp_resource_handle_t* handle);

bsp_error_t bsp_free_resource(bsp_resource_handle_t handle);

uint64_t bsp_get_timestamp_us(void);

bool bsp_is_initialized(void);

#endif /* BSP_H */
