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

// -----------------------------------------------------------------------------
// 1. Версия BSP
// -----------------------------------------------------------------------------
#define BSP_VERSION_MAJOR  1
#define BSP_VERSION_MINOR  0
#define BSP_VERSION_PATCH  0
#define BSP_VERSION_STRING "1.0.0"

// -----------------------------------------------------------------------------
// 2. Расширенные коды ошибок (для Marvell-требований)
// -----------------------------------------------------------------------------
typedef enum {
    BSP_SUCCESS = 0,
    BSP_ERROR_INVALID_PARAM,
    BSP_ERROR_NOT_INITIALIZED,
    BSP_ERROR_RESOURCE_UNAVAILABLE,
    BSP_ERROR_IO,
    BSP_ERROR_TIMEOUT,
    BSP_ERROR_NOT_SUPPORTED,
    BSP_ERROR_PORT_NOT_FOUND,
    BSP_ERROR_CONFIG_LOCKED,
    BSP_ERROR_INTERNAL = 100,      // уже используется в коде bsp_get_config
    BSP_ERROR_HARDWARE_FAULT,      // новый код
    BSP_ERROR_DRIVER_ERROR,        // новый код
    BSP_ERROR_UNKNOWN
} bsp_error_t;

// -----------------------------------------------------------------------------
// 3. Типы платы и параметры порта
// -----------------------------------------------------------------------------
typedef enum {
    BSP_BOARD_TYPE_GENERIC,
    BSP_BOARD_TYPE_SMALL,      // Small switch (e.g., 8 ports)
    BSP_BOARD_TYPE_MEDIUM,     // Medium switch (e.g., 24 ports)
    BSP_BOARD_TYPE_LARGE,      // Large switch (e.g., 48 ports)
    BSP_BOARD_TYPE_DATACENTER  // High-density datacenter switch
} bsp_board_type_t;

typedef enum {
    BSP_PORT_SPEED_10M   = 10,
    BSP_PORT_SPEED_100M  = 100,
    BSP_PORT_SPEED_1G    = 1000,
    BSP_PORT_SPEED_10G   = 10000,
    BSP_PORT_SPEED_25G   = 25000,
    BSP_PORT_SPEED_40G   = 40000,
    BSP_PORT_SPEED_100G  = 100000
} bsp_port_speed_t;

typedef enum {
    BSP_PORT_DUPLEX_HALF,
    BSP_PORT_DUPLEX_FULL
} bsp_port_duplex_t;

// -----------------------------------------------------------------------------
// 4. Структуры статуса и конфигурации
// -----------------------------------------------------------------------------
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

/**
 * @brief Структура для возврата общего статуса BSP
 */
typedef struct {
    bool     initialized;     // true, если bsp_init() уже вызывался
    uint32_t port_count;      // текущее количество портов
    uint32_t active_ports;    // число портов в состоянии UP
    // Новые поля: информация о версии BSP
    uint32_t version_major;   
    uint32_t version_minor;
    uint32_t version_patch;
} bsp_status_t;

// -----------------------------------------------------------------------------
// 5. Тип «handle» для ресурсов
// -----------------------------------------------------------------------------
typedef void* bsp_resource_handle_t;

// -----------------------------------------------------------------------------
// 6. Макросы валидации (расположены здесь, после типов, но до объявлений функций)
// -----------------------------------------------------------------------------
#define BSP_VALIDATE_PORT_ID(port_id) \
    ((port_id) < bsp_get_current_config()->num_ports)

#define BSP_IS_VALID_SPEED(speed) \
    ((speed) >= BSP_PORT_SPEED_10M && (speed) <= BSP_PORT_SPEED_100G)

#define BSP_IS_VALID_BOARD_TYPE(type) \
    ((type) >= BSP_BOARD_TYPE_GENERIC && (type) <= BSP_BOARD_TYPE_DATACENTER)

// -----------------------------------------------------------------------------
// 7. Прототипы функций BSP
// -----------------------------------------------------------------------------

/**
 * @brief Initialize the Board Support Package
 *
 * @param[in] config Указатель на структуру конфигурации платы
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_init(const bsp_config_t* config);

/**
 * @brief Deinitialize the Board Support Package and release resources
 *
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_deinit(void);

/**
 * @brief Get the default configuration for указанного типа платы
 *
 * @param[in]  board_type Тип платы (см. bsp_board_type_t)
 * @param[out] config     Заполняемый объект bsp_config_t
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config);

/**
 * @brief Get the current BSP configuration (копирует в переданный буфер)
 *
 * @param[out] config Указатель на bsp_config_t, куда будет записана конфигурация
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_NOT_INITIALIZED, BSP_ERROR_INVALID_PARAM)
 */
bsp_error_t bsp_get_config(bsp_config_t* config);

/**
 * @brief Set the BSP configuration (позволяет изменить настройки «на лету»)
 *
 * @param[in] config Указатель на новую конфигурацию
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_CONFIG_LOCKED, BSP_ERROR_INVALID_PARAM)
 */
bsp_error_t bsp_set_config(const bsp_config_t* config);

/**
 * @brief Perform a reset of the BSP using the current configuration
 *
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_reset(bool hard_reset);

/**
 * @brief Initialize a specific port
 *
 * @param[in] port_id Идентификатор порта (0 ... num_ports-1)
 * @param[in] speed   Скорость порта (см. bsp_port_speed_t)
 * @param[in] duplex  Режим дуплекса (BSP_PORT_DUPLEX_HALF или BSP_PORT_DUPLEX_FULL)
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_PORT_NOT_FOUND, BSP_ERROR_INVALID_PARAM)
 */
bsp_error_t bsp_port_init(uint32_t port_id, bsp_port_speed_t speed, bsp_port_duplex_t duplex);

/**
 * @brief Get current status of a port
 *
 * @param[in]  port_id Идентификатор порта
 * @param[out] status  Указатель на bsp_port_status_t, куда запишется статус
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_port_get_status(uint32_t port_id, bsp_port_status_t* status);

/**
 * @brief Enable or disable a given port
 *
 * @param[in] port_id Идентификатор порта
 * @param[in] enable  true = включить, false = отключить
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_port_set_enabled(uint32_t port_id, bool enable);

/**
 * @brief Register a callback для мониторинга изменения статуса порта
 *
 * @param[in] port_id  Идентификатор порта
 * @param[in] callback Функция-обработчик: void (*callback)(uint32_t port_id, bsp_port_status_t status, void* user_data)
 * @param[in] user_data Указатель, который будет передаваться в callback
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_port_register_callback(uint32_t port_id,
                                       void (*callback)(uint32_t port_id, bsp_port_status_t status, void* user_data),
                                       void* user_data);

/**
 * @brief Allocate a generic resource (например, буфер или дескриптор)
 *
 * @param[in]  resource_type Произвольный код типа ресурса
 * @param[in]  size          Требуемый размер (в байтах)
 * @param[out] handle        Указатель на переменную bsp_resource_handle_t, в которую запишется дескриптор
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_RESOURCE_UNAVAILABLE)
 */
bsp_error_t bsp_allocate_resource(uint32_t resource_type, uint32_t size, bsp_resource_handle_t* handle);

/**
 * @brief Free previously allocated resource
 *
 * @param[in] handle Дескриптор, полученный от bsp_allocate_resource
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_free_resource(bsp_resource_handle_t handle);

/**
 * @brief Get current timestamp in microseconds
 *
 * @return Timestamp (в микросекундах)
 */
uint64_t bsp_get_timestamp_us(void);

/**
 * @brief Check if BSP инициализирован
 *
 * @return true, если bsp_init() уже был вызван и успешно завершён
 */
bool bsp_is_initialized(void);

/**
 * @brief Helper function для макросов валидации:
 *        возвращает указатель на Current Configuration
 *
 * @return Указатель на statically stored bsp_config_t (если BSP инициализирован), иначе NULL
 */
const bsp_config_t* bsp_get_current_config(void);

/**
 * @brief Get the version of the BSP as строку
 *
 * @return Строка вида "MAJOR.MINOR.PATCH"
 */
const char* bsp_get_version(void);

/**
 * @brief Get detailed status of BSP (initialized, количество портов, число активных и версия)
 *
 * @param[out] status Указатель на bsp_status_t
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_INVALID_PARAM)
 */
bsp_error_t bsp_get_status(bsp_status_t* status);

#endif /* BSP_H */
