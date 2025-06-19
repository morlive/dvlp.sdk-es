
#ifndef SWITCH_SIM_SAI_ADAPTER_H
#define SWITCH_SIM_SAI_ADAPTER_H
////////SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../common/error_codes.h"
#include "../common/types.h"
#include "../hal/hw_resources.h"       // Для доступа к типу hw_context_t
#include "../hal/port.h"
#include "../l3/ip.h"                  // Для доступа к типу ip_addr_t

// Максимальное количество поддерживаемых объектов
#define SAI_MAX_PORTS 256
#define SAI_MAX_VLANS 4096
#define SAI_MAX_ROUTER_INTERFACES 128
#define SAI_MAX_ACL_TABLES 64
#define SAI_MAX_QUEUES_PER_PORT 8

/** 
 * @brief Расширенные типы объектов SAI с более детальной спецификацией
 */
typedef enum {
    SAI_OBJECT_TYPE_PORT,
    SAI_OBJECT_TYPE_VLAN,
    SAI_OBJECT_TYPE_ROUTER_INTERFACE,
    SAI_OBJECT_TYPE_ROUTE,
    SAI_OBJECT_TYPE_NEXT_HOP,
    SAI_OBJECT_TYPE_ACL_TABLE,
    SAI_OBJECT_TYPE_QOS_MAP,
    SAI_OBJECT_TYPE_TUNNEL,
    SAI_OBJECT_TYPE_MIRROR_SESSION,
    SAI_OBJECT_TYPE_HOSTIF
} sai_object_type_t;

/** 
 * @brief Расширенные режимы работы порта 
 */
typedef enum {
    SAI_PORT_MODE_L2,
    SAI_PORT_MODE_L3,
    SAI_PORT_MODE_HYBRID,
    SAI_PORT_MODE_MONITORING
} sai_port_mode_t;

// Detailed SAI Error Codes
typedef enum {
    SAI_ERROR_SUCCESS = 0,
    SAI_ERROR_GENERIC,
    SAI_ERROR_MEMORY,
    SAI_ERROR_INVALID_PARAMETER,
    SAI_ERROR_OBJECT_NOT_FOUND,
    SAI_ERROR_RESOURCE_EXHAUSTED,
    SAI_ERROR_PERMISSION_DENIED,
    SAI_ERROR_UNSUPPORTED_ATTRIBUTE,
    SAI_ERROR_THREAD_ERROR
} sai_error_code_t;

/**
 * @brief SAI status codes
 *
 * Defines standard status codes returned by SAI API functions.
 * Compatible with standard SAI specifications.
 */
typedef enum _sai_status_t {
    SAI_STATUS_SUCCESS                  = 0x00000000,  /**< Operation successful */
    
    /* Error codes */
    SAI_STATUS_FAILURE                  = 0x00000001,  /**< Generic failure */
    SAI_STATUS_NOT_IMPLEMENTED          = 0x00000002,  /**< Function not implemented */
    SAI_STATUS_NOT_SUPPORTED            = 0x00000003,  /**< Function not supported */
    SAI_STATUS_INVALID_PARAMETER        = 0x00000004,  /**< Invalid parameter */
    SAI_STATUS_INSUFFICIENT_RESOURCES   = 0x00000005,  /**< Insufficient resources */
    SAI_STATUS_INVALID_PORT_NUMBER      = 0x00000006,  /**< Invalid port number */
    SAI_STATUS_INVALID_PORT_MEMBER      = 0x00000007,  /**< Invalid port member */
    SAI_STATUS_INVALID_VLAN_ID          = 0x00000008,  /**< Invalid VLAN ID */
    SAI_STATUS_UNINITIALIZED            = 0x00000009,  /**< SAI not initialized */
    SAI_STATUS_TABLE_FULL               = 0x0000000A,  /**< Table is full */
    SAI_STATUS_ITEM_ALREADY_EXISTS      = 0x0000000B,  /**< Item already exists */
    SAI_STATUS_ITEM_NOT_FOUND           = 0x0000000C,  /**< Item not found */
    SAI_STATUS_BUFFER_OVERFLOW          = 0x0000000D,  /**< Buffer overflow */
    SAI_STATUS_INVALID_QOS_MAP_ID       = 0x0000000E,  /**< Invalid QoS map ID */
    SAI_STATUS_INVALID_ACL_TABLE_ID     = 0x0000000F,  /**< Invalid ACL table ID */
    SAI_STATUS_INVALID_ATTRIBUTE        = 0x00000010,  /**< Invalid attribute */
    SAI_STATUS_ATTR_NOT_IMPLEMENTED     = 0x00000011,  /**< Attribute not implemented */
    SAI_STATUS_UNKNOWN_ATTRIBUTE        = 0x00000012,  /**< Unknown attribute */
    SAI_STATUS_ATTR_NOT_SUPPORTED       = 0x00000013,  /**< Attribute not supported */
    SAI_STATUS_ATTR_OUT_OF_RANGE        = 0x00000014,  /**< Attribute value out of range */
    SAI_STATUS_ATTR_READ_ONLY           = 0x00000015,  /**< Attribute is read-only */
    SAI_STATUS_INVALID_OBJECT_TYPE      = 0x00000016,  /**< Invalid object type */
    SAI_STATUS_INVALID_OBJECT_ID        = 0x00000017,  /**< Invalid object ID */
    SAI_STATUS_OBJECT_IN_USE            = 0x00000018,  /**< Object is in use */
    SAI_STATUS_INVALID_SWITCH_ID        = 0x00000019,  /**< Invalid switch ID */
    SAI_STATUS_RESOURCE_IN_USE          = 0x0000001A,  /**< Resource is in use */
    SAI_STATUS_ADDR_NOT_FOUND           = 0x0000001B,  /**< Address not found */
    SAI_STATUS_INVALID_BRIDGE_ID        = 0x0000001C,  /**< Invalid bridge ID */
    SAI_STATUS_INVALID_ROUTE            = 0x0000001D,  /**< Invalid route */
    SAI_STATUS_INVALID_NEXT_HOP         = 0x0000001E,  /**< Invalid next hop */
    SAI_STATUS_MUTEX_FAILURE            = 0x0000001F,  /**< Mutex operation failed */
    SAI_STATUS_API_LOCK_TIMEOUT         = 0x00000020,  /**< API lock timeout */
    SAI_STATUS_MAC_ADDRESS_FAILURE      = 0x00000021,  /**< MAC address failure */
    SAI_STATUS_PORT_STATS_FAILURE       = 0x00000022,  /**< Port statistics failure */
} sai_status_t;

/**
 * @brief Check if a SAI status code indicates success
 *
 * @param status The SAI status code to check
 * @return true if the status indicates success, false otherwise
 */
static inline bool SAI_STATUS_IS_SUCCESS(sai_status_t status) {
    return (status == SAI_STATUS_SUCCESS);
}

/**
 * @brief Get printable string for SAI status code
 *
 * @param status The SAI status code
 * @return Printable string for the status code
 */
const char* sai_status_to_string(sai_status_t status);

/**
 * @brief Расширенная конфигурация порта с QoS и безопасностью
 */
typedef struct {
    port_id_t port_id;
    port_speed_t speed;
    bool admin_state;
    port_duplex_t duplex;
    uint16_t mtu;
    bool learning_enabled;
    sai_port_mode_t port_mode;
    
    // QoS настройки
    uint8_t default_cos;
    uint8_t default_dscp;
    
    // Настройки безопасности
    bool storm_control_enabled;
    uint64_t storm_control_rate_pps;
    
    // Расширенные фильтры
    bool dhcp_filter;
    bool arp_filter;
} sai_port_config_t;

/**
 * @brief Расширенная конфигурация VLAN с политиками
 */
typedef struct {
    vlan_id_t vlan_id;
    char name[64];
    bool is_flood_disabled;
    bool is_private_vlan;
    uint8_t priority;
    
    // Политики безопасности
    bool isolation_enabled;
    port_id_t allowed_ports[SAI_MAX_PORTS];
    size_t allowed_ports_count;
} sai_vlan_config_t;

/**
 * @brief Расширенная конфигурация маршрутизации
 */
typedef struct {
    uint32_t router_interface_id;
    port_id_t port_id;
    mac_addr_t mac_address;
    ip_addr_t ip_address;
    ip_addr_t subnet_mask;
    
    // Дополнительные параметры маршрутизации
    bool is_virtual_router;
    uint32_t vrf_id;
    bool rpf_enabled;
} sai_router_interface_config_t;

/**
 * @brief Структура для мониторинга производительности SAI
 */
typedef struct {
    uint64_t total_objects_created;
    uint64_t total_objects_destroyed;
    uint64_t total_reconfigurations;
    
    // Статистика по типам объектов
    struct {
        uint32_t current_count;
        uint32_t max_count;
    } object_stats[10];  // Соответствует sai_object_type_t
} sai_performance_metrics_t;

// Типы callback-функций
typedef void (*sai_object_create_callback)(sai_object_type_t type, uint32_t object_id);
typedef void (*sai_object_remove_callback)(sai_object_type_t type, uint32_t object_id);
typedef void (*sai_attribute_change_callback)(sai_object_type_t type, uint32_t object_id, const char *attr_name);

/**
 * @brief Менеджер callback-функций SAI
 */
typedef struct {
    sai_object_create_callback on_object_create;
    sai_object_remove_callback on_object_remove;
    sai_attribute_change_callback on_attribute_change;
} sai_callback_manager_t;

// Основные функции SAI адаптера
/**
 * @brief Thread-safe SAI adapter initialization
 * @return Thread-safe status of initialization
 */
status_t sai_adapter_init(hw_context_t *hw_context);

/**
 * @brief Thread-safe SAI adapter cleanup
 * @return Thread-safe status of cleanup
 */
status_t sai_adapter_deinit(void);

/**
 * @brief Configure port with advanced settings and thread-safe operations
 * @param config Advanced port configuration
 * @return Thread-safe status of port configuration
 */
sai_status_t sai_configure_port_advanced(const sai_port_config_t *config);

// Расширенные функции конфигурации
status_t sai_create_vlan_advanced(const sai_vlan_config_t *config);
status_t sai_create_router_interface_advanced(const sai_router_interface_config_t *config);

// Функции управления объектами
status_t sai_add_port_to_vlan(vlan_id_t vlan_id, port_id_t port_id, bool is_tagged);
status_t sai_remove_port_from_vlan(vlan_id_t vlan_id, port_id_t port_id);

// Функции мониторинга и производительности
status_t sai_get_performance_metrics(sai_performance_metrics_t *metrics);

// Callback-менеджмент
status_t sai_register_callbacks(const sai_callback_manager_t *callbacks);

// Расширенные функции работы с атрибутами
status_t sai_get_attribute_advanced(
    sai_object_type_t object_type, 
    uint32_t object_id, 
    const char *attribute_name, 
    void *value,
    size_t *value_size
);

status_t sai_set_attribute_advanced(
    sai_object_type_t object_type, 
    uint32_t object_id, 
    const char *attribute_name, 
    const void *value,
    size_t value_size
);

#endif /* SWITCH_SIM_SAI_ADAPTER_H */

