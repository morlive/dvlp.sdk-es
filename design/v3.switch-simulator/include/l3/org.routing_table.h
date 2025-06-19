/**
 * @file routing_table.h
 * @brief Структуры данных и функции для работы с таблицей маршрутизации
 * 
 * Этот файл содержит определения типов данных и функций для создания,
 * управления и доступа к таблице маршрутизации в симуляторе коммутатора.
 */

#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include "../common/types.h"
#include "../common/error_codes.h"
#include "../hal/port.h"
#include "../l3/ip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Максимальное количество записей в таблице маршрутизации */
#define MAX_ROUTES 1024

/** Максимальная длина имени сетевого интерфейса */
#define MAX_INTERFACE_NAME_LEN 32

/**
 * @brief Типы протоколов маршрутизации
 */
typedef enum {
    ROUTE_TYPE_STATIC,    /**< Статический маршрут */
    ROUTE_TYPE_CONNECTED, /**< Непосредственно подключенная сеть */
    ROUTE_TYPE_RIP,       /**< Маршрут получен с помощью RIP */
    ROUTE_TYPE_OSPF,      /**< Маршрут получен с помощью OSPF */
    ROUTE_TYPE_BGP        /**< Маршрут получен с помощью BGP */
} route_type_t;

/**
 * @brief Административное расстояние для разных типов маршрутов
 */
typedef enum {
    ADMIN_DISTANCE_CONNECTED = 0,    /**< Непосредственно подключенная сеть */  
    ADMIN_DISTANCE_STATIC = 1,       /**< Статический маршрут */
    ADMIN_DISTANCE_OSPF = 110,       /**< OSPF маршрут */
    ADMIN_DISTANCE_RIP = 120,        /**< RIP маршрут */
    ADMIN_DISTANCE_BGP_EXTERNAL = 20, /**< Внешний BGP маршрут */
    ADMIN_DISTANCE_BGP_INTERNAL = 200 /**< Внутренний BGP маршрут */
} admin_distance_t;

/**
 * @brief Структура, представляющая запись в таблице маршрутизации
 */
typedef struct {
    ipv4_addr_t destination;      /**< IP-адрес назначения */
    ipv4_addr_t netmask;          /**< Маска подсети */
    ipv4_addr_t gateway;          /**< IP-адрес шлюза */
    uint16_t interface_index;   /**< Индекс исходящего интерфейса */

//    uint16_t ingress_port;        /**< Входной порт, с которого пришел пакет */
//    uint16_t egress_port;         /**< Выходной порт для отправки пакета */
    
    char interface_name[MAX_INTERFACE_NAME_LEN]; /**< Имя исходящего интерфейса */
    route_type_t type;          /**< Тип маршрута */
    uint8_t admin_distance;     /**< Административное расстояние */
    uint16_t metric;            /**< Метрика маршрута */
    bool active;                /**< Активен ли маршрут */
    uint32_t timestamp;         /**< Время последнего обновления маршрута */
    bool is_ipv6;                 /**< Флаг IPv6 маршрута */
} route_entry_t;

/**
 * @brief Структура, представляющая таблицу маршрутизации
 */
typedef struct {
    route_entry_t routes[MAX_ROUTES]; /**< Массив записей маршрутизации */
    uint32_t route_count;             /**< Количество записей в таблице */
    uint32_t last_update_time;        /**< Время последнего обновления таблицы */
    bool changed;                     /**< Флаг изменения таблицы */
} routing_table_t;


/**
 * @brief Получить экземпляр глобальной таблицы маршрутизации.
 *
 * Функция возвращает указатель на глобальный экземпляр таблицы маршрутизации.
 * Если таблица не была инициализирована, функция выполнит инициализацию.
 *
 * @return Указатель на таблицу маршрутизации или NULL в случае ошибки.
 */
routing_table_t *routing_table_get_instance(void);

/**
 * @brief Инициализирует таблицу маршрутизации
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_init(routing_table_t *table);

/**
 * @brief Clean up the routing table module resources
 *
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_cleanup(void);

/**
 * @brief Добавляет новый маршрут в таблицу маршрутизации
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @param route Указатель на запись маршрута для добавления
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_add_route(routing_table_t *table, const route_entry_t *route);

/**
 * @brief Удаляет маршрут из таблицы маршрутизации
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @param destination IP-адрес назначения
 * @param netmask Маска подсети
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_remove_route(routing_table_t *table, ipv4_addr_t destination, ipv4_addr_t netmask);

////**
/// * @brief Находит наилучший маршрут для указанного IP-адреса назначения
/// * 
/// * @param table Указатель на структуру таблицы маршрутизации
/// * @param destination IP-адрес назначения
/// * @param route Указатель на структуру для сохранения найденного маршрута
/// * @return status_t Код ошибки (OK в случае успеха, NOT_FOUND если маршрут не найден)
/// */
///status_t routing_table_lookup(const routing_table_t *table, ipv4_addr_t destination, route_entry_t *route);

/**
 * @brief Look up a route based on destination IP in the specified routing table
 *
 * @param routing_table Pointer to the routing table instance
 * @param dest_ip Destination IP address
 * @param type IP address type (IPv4 or IPv6)
 * @param route_info Pointer to store the found route information
 * @return STATUS_SUCCESS if successful, error code otherwise
 */
///status_t routing_table_lookup(routing_table_t *routing_table, const ip_addr_t *dest_ip, 
///    ip_addr_type_t type, routing_entry_t *route_info);
status_t routing_table_lookup(routing_table_t *routing_table, const ip_addr_t *dest_ip, 
    ip_addr_type_t type, route_entry_t *route_info);

/**
 * @brief Обновляет существующий маршрут в таблице маршрутизации
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @param route Указатель на обновленную запись маршрута
 * @return status_t Код ошибки (OK в случае успеха, NOT_FOUND если маршрут не найден)
 */
status_t routing_table_update_route(routing_table_t *table, const route_entry_t *route);

/**
 * @brief Удаляет все маршруты указанного типа
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @param type Тип маршрута для удаления
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_clear_routes_by_type(routing_table_t *table, route_type_t type);

/**
 * @brief Получает количество маршрутов в таблице
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @return uint32_t Количество маршрутов
 */
uint32_t routing_table_get_count(const routing_table_t *table);

/**
 * @brief Получает все маршруты указанного типа
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @param type Тип маршрута для фильтрации
 * @param routes Массив для сохранения найденных маршрутов
 * @param max_routes Максимальное количество маршрутов для возврата
 * @param actual_routes Указатель для сохранения фактического количества найденных маршрутов
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_get_routes_by_type(const routing_table_t *table, route_type_t type, 
                                           route_entry_t *routes, uint32_t max_routes, uint32_t *actual_routes);

/**
 * @brief Копирует всю таблицу маршрутизации
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @param routes Массив для сохранения всех маршрутов
 * @param max_routes Максимальное количество маршрутов для возврата
 * @param actual_routes Указатель для сохранения фактического количества скопированных маршрутов
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_get_all_routes(const routing_table_t *table, route_entry_t *routes, 
                                        uint32_t max_routes, uint32_t *actual_routes);

/**
 * @brief Очищает всю таблицу маршрутизации
 * 
 * @param table Указатель на структуру таблицы маршрутизации
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_clear(routing_table_t *table);

/**
 * @brief Создает статический маршрут
 * 
 * @param destination IP-адрес назначения
 * @param netmask Маска подсети
 * @param gateway IP-адрес шлюза
 * @param interface_index Индекс исходящего интерфейса
 * @param interface_name Имя исходящего интерфейса
 * @param metric Метрика маршрута
 * @param route Указатель на структуру для сохранения созданного маршрута
 * @return status_t Код ошибки (OK в случае успеха)
 */
status_t routing_table_create_static_route(ipv4_addr_t destination, ipv4_addr_t netmask,
                                             ipv4_addr_t gateway, uint16_t interface_index,
                                             const char *interface_name, uint16_t metric,
                                             route_entry_t *route);

/**
 * @brief Вычисляет префикс сети на основе IP-адреса и маски
 * 
 * @param ip IP-адрес
 * @param netmask Маска подсети
 * @return ipv4_addr_t Префикс сети
 */
ipv4_addr_t routing_table_calculate_network(ipv4_addr_t ip, ipv4_addr_t netmask);

/**
 * @brief Возвращает длину префикса для данной маски подсети
 * 
 * @param netmask Маска подсети
 * @return uint8_t Длина префикса (количество единичных битов в маске)
 */
uint8_t routing_table_get_prefix_length(ipv4_addr_t netmask);

/**
 * @brief Создает маску подсети на основе длины префикса
 * 
 * @param prefix_length Длина префикса (количество единичных битов)
 * @return ipv4_addr_t Маска подсети
 */
ipv4_addr_t routing_table_create_netmask(uint8_t prefix_length);

#ifdef __cplusplus
}
#endif

#endif /* ROUTING_TABLE_H */
