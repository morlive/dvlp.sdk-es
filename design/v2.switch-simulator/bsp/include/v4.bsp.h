/**
 * @file bsp.h
 * @brief Board Support Package main interface header
 *
 * Этот файл содержит публичные объявления для BSP (Board Support Package),
 * обеспечивающие абстракцию «железа» для симулятора коммутатора.
 */

#ifndef BSP_H
#define BSP_H

// -----------------------------------------------------------------------------
// 0. Подключение стандартных заголовков
// -----------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// 1. Версия BSP (мажорная, минорная, патч и строковый вид)
// -----------------------------------------------------------------------------
#define BSP_VERSION_MAJOR   1
#define BSP_VERSION_MINOR   0
#define BSP_VERSION_PATCH   0
#define BSP_VERSION_STRING  "1.0.0"

// -----------------------------------------------------------------------------
// 2. Базовые коды ошибок BSP (диапазон 0..99)
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
    BSP_ERROR_UNKNOWN
} bsp_error_t;

// -----------------------------------------------------------------------------
// 3. «Расширенные» коды ошибок (брендовые, драйверные, аппаратные) (100..199)
// -----------------------------------------------------------------------------
typedef enum {
    BSP_ERROR_INTERNAL       = 100,  // используется, например, в bsp_get_config()
    BSP_ERROR_HARDWARE_FAULT,        // сбой железа или эмуляции
    BSP_ERROR_DRIVER_ERROR            // ошибка низкоуровневого драйвера
} bsp_extended_error_t;

// -----------------------------------------------------------------------------
// 4. Типы платы и скорость/дуплекс порта
// -----------------------------------------------------------------------------
typedef enum {
    BSP_BOARD_TYPE_GENERIC,
    BSP_BOARD_TYPE_SMALL,      // небольшая плата (≈ 8 портов)
    BSP_BOARD_TYPE_MEDIUM,     // средняя плата (≈ 24 порта)
    BSP_BOARD_TYPE_LARGE,      // большая плата (≈ 48 портов)
    BSP_BOARD_TYPE_DATACENTER  // дата-центровая плата, высокая плотность портов
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
// 5. Структуры статуса порта и конфигурации платы
// -----------------------------------------------------------------------------
typedef struct {
    bool               link_up;              // true если линк поднят
    bsp_port_speed_t   speed;                // скорость порта
    bsp_port_duplex_t  duplex;               // дуплексный режим
    bool               flow_control_enabled; // включён ли flow control
    uint64_t           rx_bytes;             // принятые байты
    uint64_t           tx_bytes;             // переданные байты
    uint64_t           rx_packets;           // принятые пакеты
    uint64_t           tx_packets;           // переданные пакеты
    uint64_t           rx_errors;            // ошибки при приёме
    uint64_t           tx_errors;            // ошибки при передаче
} bsp_port_status_t;

typedef struct {
    bsp_board_type_t board_type;        // тип платы (enum выше)
    uint32_t         num_ports;         // общее число портов
    uint32_t         cpu_frequency_mhz; // частота CPU (например, симулированная)
    uint32_t         memory_size_mb;    // объём памяти (MB)
    bool             has_layer3_support;// поддержка L3 (true/false)
    bool             has_qos_support;   // поддержка QoS
    bool             has_acl_support;   // поддержка ACL
    const char*      board_name;        // человекочитаемое имя платы
} bsp_config_t;

/**
 * @brief Структура для возврата общего статуса BSP
 *
 * Используется в bsp_get_status(). Содержит флаги, количество портов, число активных,
 * а также версию BSP (major/minor/patch).
 */
typedef struct {
    bool     initialized;    // true, если bsp_init() уже выполнен
    uint32_t port_count;     // текущее число портов
    uint32_t active_ports;   // число портов, находящихся в состоянии UP
    // Информация о версии BSP:
    uint32_t version_major;  
    uint32_t version_minor;  
    uint32_t version_patch;  
} bsp_status_t;

// -----------------------------------------------------------------------------
// 6. Тип «handle» для ресурсов (opaque pointer)
// -----------------------------------------------------------------------------
typedef void* bsp_resource_handle_t;

// -----------------------------------------------------------------------------
// 7. Макросы валидации параметров (их можно не вызывать сразу, но они уже определены)
// -----------------------------------------------------------------------------
#define BSP_VALIDATE_PORT_ID(port_id) \
    ((port_id) < bsp_get_current_config()->num_ports)

#define BSP_IS_VALID_SPEED(speed) \
    (((speed) >= BSP_PORT_SPEED_10M) && ((speed) <= BSP_PORT_SPEED_100G))

#define BSP_IS_VALID_BOARD_TYPE(type) \
    (((type) >= BSP_BOARD_TYPE_GENERIC) && ((type) <= BSP_BOARD_TYPE_DATACENTER))

// -----------------------------------------------------------------------------
// 8. Прототипы функций BSP-API
// -----------------------------------------------------------------------------

/**
 * @brief Initialize the Board Support Package
 *
 * @param[in] config Указатель на структуру конфигурации платы
 * @return BSP_SUCCESS или код ошибки из bsp_error_t (может быть приведён из bsp_extended_error_t)
 */
bsp_error_t bsp_init(const bsp_config_t* config);

/**
 * @brief Deinitialize the Board Support Package and release all resources
 *
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_deinit(void);

/**
 * @brief Get default configuration для указанного типа платы
 *
 * @param[in]  board_type Тип платы (enum bsp_board_type_t)
 * @param[out] config     Заполняемый экземпляр bsp_config_t
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config);

/**
 * @brief Get current BSP configuration (копирует данные в переданный буфер)
 *
 * @param[out] config Указатель на bsp_config_t, куда нужно записать текущую конфигурацию
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_NOT_INITIALIZED)
 */
bsp_error_t bsp_get_config(bsp_config_t* config);

/**
 * @brief Set (изменить) BSP configuration «на лету»
 *
 * @param[in] config Указатель на новую конфигурацию
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_CONFIG_LOCKED)
 */
bsp_error_t bsp_set_config(const bsp_config_t* config);

/**
 * @brief Reset BSP to default state, используя текущее конфигурирование
 *
 * @param[in] hard_reset true = полный сброс, false = программный reset
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_reset(bool hard_reset);

/**
 * @brief Initialize a specific port
 *
 * @param[in] port_id Идентификатор порта (0 ... num_ports-1)
 * @param[in] speed   Скорость порта (enum bsp_port_speed_t)
 * @param[in] duplex  Режим дуплекса (enum bsp_port_duplex_t)
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_PORT_NOT_FOUND)
 */
bsp_error_t bsp_port_init(uint32_t port_id, bsp_port_speed_t speed, bsp_port_duplex_t duplex);

/**
 * @brief Get the current status of a port
 *
 * @param[in]  port_id Идентификатор порта
 * @param[out] status  Указатель на bsp_port_status_t
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_port_get_status(uint32_t port_id, bsp_port_status_t* status);

/**
 * @brief Enable or disable a given port
 *
 * @param[in] port_id Идентификатор порта
 * @param[in] enable  true = включить порт, false = отключить
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_port_set_enabled(uint32_t port_id, bool enable);

/**
 * @brief Register a callback для уведомлений об изменении статуса порта
 *
 * @param[in] port_id   Идентификатор порта
 * @param[in] callback  Функция: void callback(uint32_t port_id, bsp_port_status_t status, void* user_data)
 * @param[in] user_data Пользовательские данные, передаются в callback
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_port_register_callback(uint32_t port_id,
                                       void (*callback)(uint32_t port_id,
                                                        bsp_port_status_t status,
                                                        void* user_data),
                                       void* user_data);

/**
 * @brief Allocate a generic resource (например, буфер, дескриптор) в BSP
 *
 * @param[in]  resource_type Произвольный код ресурса
 * @param[in]  size          Размер в байтах
 * @param[out] handle        Выходной дескриптор (opaque pointer)
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_RESOURCE_UNAVAILABLE)
 */
bsp_error_t bsp_allocate_resource(uint32_t resource_type, uint32_t size, bsp_resource_handle_t* handle);

/**
 * @brief Free previously allocated resource
 *
 * @param[in] handle Дескриптор, полученный от bsp_allocate_resource()
 * @return BSP_SUCCESS или код ошибки
 */
bsp_error_t bsp_free_resource(bsp_resource_handle_t handle);

/**
 * @brief Get current timestamp in microseconds
 *
 * @return Таймстамп (в микросекундах)
 */
uint64_t bsp_get_timestamp_us(void);

/**
 * @brief Check if BSP уже инициализирован (bsp_init() был вызван и вернул BSP_SUCCESS)
 *
 * @return true, если BSP инициализирован, иначе false
 */
bool bsp_is_initialized(void);

/**
 * @brief Helper function для макросов валидации:
 *        возвращает указатель на «текущую» конфигурацию BSP
 *
 * @return Указатель на статическое bsp_config_t (если BSP инициализирован), иначе NULL
 */
const bsp_config_t* bsp_get_current_config(void);

/**
 * @brief Get BSP version as строку
 *
 * @return Строка вида "MAJOR.MINOR.PATCH"
 */
const char* bsp_get_version(void);

/**
 * @brief Get detailed status of BSP (initialized, port_count, active_ports, version)
 *
 * @param[out] status Указатель на bsp_status_t, который будет заполнен
 * @return BSP_SUCCESS или код ошибки (например, BSP_ERROR_INVALID_PARAM)
 */
bsp_error_t bsp_get_status(bsp_status_t* status);

#endif /* BSP_H */
