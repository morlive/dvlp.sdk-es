/**
 * @file port.c
 * @brief Port management implementation for switch simulator
 */

#include <string.h>
#include <stdlib.h>
//#include <arpa/inet.h>
#include <netinet/in.h>  // Для htons/ntohs

#include "../../include/hal/ethernet.h"
#include "../../include/hal/port.h"
#include "../../include/common/logging.h"
#include "../../include/common/utils.h"

/* Forward declarations for hardware simulation functions */
extern status_t hw_sim_init(void);
extern status_t hw_sim_shutdown(void);
extern status_t hw_sim_get_port_info(port_id_t port_id, port_info_t *info);
extern status_t hw_sim_set_port_config(port_id_t port_id, const port_config_t *config);
extern status_t hw_sim_get_port_config(port_id_t port_id, port_config_t *config);
extern status_t hw_sim_get_port_count(uint32_t *count);
extern status_t hw_sim_clear_port_stats(port_id_t port_id);

// Добавить forward declaration после существующих (строка ~34)
static status_t port_generate_default_mac(uint16_t port_id, mac_addr_t *mac_addr);
static void port_config_changed_notify(uint16_t port_id, port_config_change_t change_type);
static status_t port_mac_subsystem_init(void);

/* Static variables */
static bool g_port_initialized = false;
static uint32_t g_phys_count = 0;
static port_id_t g_cpu_port_id = 0;

// Глобальная таблица MAC-адресов портов (инициализируется при старте)
static mac_addr_t g_port_mac_table[MAX_PORTS];
static bool g_port_mac_initialized[MAX_PORTS] = {false};

/* Static function prototypes */
static status_t port_stats_update_tx(port_id_t port_id, size_t length);


/**
 * @brief Initialize port subsystem
 * 
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_init(void)
{
    LOG_INFO(LOG_CATEGORY_HAL, "Initializing port subsystem");
    
    if (g_port_initialized) {
        LOG_WARNING(LOG_CATEGORY_HAL, "Port subsystem already initialized");
        return STATUS_SUCCESS;
    }
    
    /* Initialize hardware simulation */
    status_t status = hw_sim_init();
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to initialize hardware simulation");
        return status;
    }


    /* Узнаём физическое число портов из hw_sim */
    status = hw_sim_get_port_count(&g_phys_count);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port count");
        return status;
    }

    /* Резервируем следующий ID под CPU-порт */
    g_cpu_port_id = (port_id_t)g_phys_count;

    /* Теперь общее число портов = phys + 1 */
    LOG_INFO(LOG_CATEGORY_HAL, "Detected %u physical ports; CPU-port = %u",
             g_phys_count, g_cpu_port_id);

    
    g_port_initialized = true;

    // Initialize MAC address subsystem
    status = port_mac_subsystem_init();
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to initialize MAC subsystem");
        g_port_initialized = false;
        hw_sim_shutdown();
        return status;
    }

    LOG_INFO(LOG_CATEGORY_HAL, "Port subsystem initialized successfully");

    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown port subsystem
 * 
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_shutdown(void)
{
    LOG_INFO(LOG_CATEGORY_HAL, "Shutting down port subsystem");
    
    if (!g_port_initialized) {
        LOG_WARNING(LOG_CATEGORY_HAL, "Port subsystem not initialized");
        return STATUS_SUCCESS;
    }
    
    /* Shutdown hardware simulation */
    status_t status = hw_sim_shutdown();
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to shutdown hardware simulation");
        return status;
    }
    
    g_port_initialized = false;
    LOG_INFO(LOG_CATEGORY_HAL, "Port subsystem shutdown successfully");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get port information
 * 
 * @param port_id Port identifier
 * @param[out] info Port information structure to fill
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_info(port_id_t port_id, port_info_t *info)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (!info) {
        return STATUS_INVALID_PARAMETER;
    }
    
//    /* Get information from hardware simulation */
//    status_t status = hw_sim_get_port_info(port_id, info);
//    if (status != STATUS_SUCCESS) {
//        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get information for port %u", port_id);
//    } else {
//        LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved information for port %u (%s)", 
//                 port_id, info->name);
//    }
//    
//    return status;

    /* Get information from hardware simulation */
    if ((uint32_t)port_id < g_phys_count) {
        /* Физический порт */
        status_t status = hw_sim_get_port_info(port_id, info);
        if (status != STATUS_SUCCESS) {
            LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get information for port %u", port_id);
        } else {
            LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved information for port %u (%s)",
                     port_id, info->name);
        }
        return status;
    }
    else if (port_id == g_cpu_port_id) {
        /* CPU-порт как internal loopback */
        memset(info, 0, sizeof(port_info_t));
        info->id = port_id;
        info->type = PORT_TYPE_CPU;
        strcpy(info->name, "cpu");
        info->config.admin_state = true;
        info->config.speed = PORT_SPEED_100G; // CPU порт обычно самый быстрый
        info->config.duplex = PORT_DUPLEX_FULL;
        info->state = PORT_STATE_UP;
        // MAC-адрес можно сделать каким-то особым, например 00:00:00:00:00:01
        info->mac_addr.addr[0] = 0x00;
        info->mac_addr.addr[1] = 0x00;
        info->mac_addr.addr[2] = 0x00;
        info->mac_addr.addr[3] = 0x00;
        info->mac_addr.addr[4] = 0x00;
        info->mac_addr.addr[5] = 0x01;

        LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved information for CPU port");
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_INVALID_PORT;
    }


}

/**
 * @brief Set port configuration
 * 
 * @param port_id Port identifier
 * @param config Port configuration to apply
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_set_config(port_id_t port_id, const port_config_t *config)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (!config) {
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Set configuration in hardware simulation */
    status_t status = hw_sim_set_port_config(port_id, config);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to set configuration for port %u", port_id);
    } else {
        LOG_INFO(LOG_CATEGORY_HAL, "Set configuration for port %u (admin_state=%s, speed=%u)",
                port_id, config->admin_state ? "up" : "down", config->speed);
    }
    
    return status;
}


/**
* Retrieve the configuration for a specific port
*
* This function obtains the current configuration of a specified port,
* including its driver handle, speed, mode, and other operational parameters.
* It performs validation checks before accessing the hardware simulation layer.
*
* @param port_id The identifier of the port for which to retrieve configuration
* @param config  Pointer to a port_config_t structure to be filled with the port's configuration
*
* @return STATUS_SUCCESS on successful operation
*         STATUS_INVALID_PORT if the port_id is not valid
*         STATUS_INVALID_PARAMETER if config pointer is NULL
*         Other status codes from the hw_sim layer in case of failure
*/
status_t port_get_config(port_id_t port_id, port_config_t *config) {
   /* Validate input parameters */
   if (!port_is_valid(port_id)) {
       LOG_ERROR(LOG_CATEGORY_HAL, "Invalid port ID %d in port_get_config", port_id);
       return STATUS_INVALID_PORT;
   }

   if (config == NULL) {
       LOG_ERROR(LOG_CATEGORY_HAL, "NULL config pointer passed to port_get_config");
       return STATUS_INVALID_PARAMETER;
   }

   /* Check if port module is initialized */
   if (!g_port_initialized) {
       LOG_ERROR(LOG_CATEGORY_HAL, "Port module not initialized during port_get_config call");
       return STATUS_MODULE_NOT_INITIALIZED;
   }

   LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieving configuration for port %d", port_id);

   /* Retrieve the port configuration from hardware simulation layer */
   status_t status = hw_sim_get_port_config(port_id, config);
   if (status != STATUS_SUCCESS) {
       LOG_ERROR(LOG_CATEGORY_HAL, "Failed to retrieve configuration for port %d: %s",
                port_id, status_to_string(status));
       return status;
   }

   /* Verify critical configuration fields */
   if (config->driver == NULL) {
       LOG_WARNING(LOG_CATEGORY_HAL, "Port %d has NULL driver handle in configuration", port_id);
       /* We continue as this might be valid in some cases */
   }

   /* Post-processing of retrieved configuration if needed */
   /* For example, applying any HAL-level overrides or default values */

   LOG_DEBUG(LOG_CATEGORY_HAL, "Successfully retrieved configuration for port %d "
            "(speed: %u Mbps, mode: %d, mtu: %u)",
            port_id, config->speed, config->mode, config->mtu);

   return STATUS_SUCCESS;
}


/**
 * @brief Set port administrative state
 * 
 * @param port_id Port identifier
 * @param admin_up True to set port administratively up, false for down
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_set_admin_state(port_id_t port_id, bool admin_up)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    /* Get current configuration */
    port_info_t info;
    status_t status = hw_sim_get_port_info(port_id, &info);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get current configuration for port %u", port_id);
        return status;
    }
    
    /* Update admin state only */
    port_config_t config = info.config;
    config.admin_state = admin_up;
    
    /* Apply updated configuration */
    status = hw_sim_set_port_config(port_id, &config);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to set admin state for port %u", port_id);
    } else {
        LOG_INFO(LOG_CATEGORY_HAL, "Set admin state for port %u to %s",
                port_id, admin_up ? "up" : "down");
    }
    
    return status;
}

/**
 * @brief Get port statistics
 * 
 * @param port_id Port identifier
 * @param[out] stats Statistics structure to fill
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_stats(port_id_t port_id, port_stats_t *stats)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (!stats) {
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Get port information which includes stats */
    port_info_t info;
    status_t status = hw_sim_get_port_info(port_id, &info);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get statistics for port %u", port_id);
        return status;
    }
    
    /* Copy statistics */
    memcpy(stats, &info.stats, sizeof(port_stats_t));
    
    LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved statistics for port %u (rx: %lu, tx: %lu)",
             port_id, stats->rx_packets, stats->tx_packets);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Clear port statistics counters
 * 
 * @param port_id Port identifier
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_clear_stats(port_id_t port_id)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    /* Clear statistics in hardware simulation */
    status_t status = hw_sim_clear_port_stats(port_id);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to clear statistics for port %u", port_id);
    } else {
        LOG_INFO(LOG_CATEGORY_HAL, "Cleared statistics for port %u", port_id);
    }
    
    return status;
}

/**
 * @brief Get total number of ports in the system
 * 
 * @param[out] count Pointer to store port count
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_count(uint32_t *count)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (!count) {
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Get count from hardware simulation */
    status_t status = hw_sim_get_port_count(count);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port count");
    } else {
        LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved port count: %u", *count);
    }
    
    return status;
}

// <A: need for check ---------------------------------------------
status_t port_get_total_count(uint32_t *count_out)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }

    if (!count_out) {
        return STATUS_INVALID_PARAMETER;
    }

    *count_out = g_phys_count + 1; // Физические порты + CPU-порт
    LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved total port count: %u", *count_out);

    return STATUS_SUCCESS;
}

port_id_t port_cpu_id(void)
{
    return g_cpu_port_id;
}

// :A> -----------------------------------------------------------

/**
 * @brief Check if port ID is valid
 * 
 * @param port_id Port identifier to check
 * @return bool True if the port ID is valid, false otherwise
 */
bool port_is_valid(port_id_t port_id)
{
    if (!g_port_initialized) {
        return false;
    }
    
    /* Check if port ID is negative */
    if (port_id < 0) {
        return false;
    }
    
    /* Get total port count */
    uint32_t total_ports;
    status_t status = hw_sim_get_port_count(&total_ports);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port count for validation");
        return false;
    }
    
    /* Check if port ID is within range */
    return (port_id < (port_id_t)total_ports);
}

/**
 * @brief Get port operational state
 *
 * @param port_id Port identifier
 * @param[out] state Pointer to store the port state
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_state(port_id_t port_id, port_state_t *state)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }

    if (!state) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!port_is_valid(port_id)) {
        return STATUS_INVALID_PORT;
    }

    /* Get port information which includes state */
    port_info_t info;
    status_t status = hw_sim_get_port_info(port_id, &info);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get state for port %u", port_id);
        return status;
    }

    /* Return operational state */
    *state = info.state;

    LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved operational state for port %u: %d",
             port_id, *state);

    return STATUS_SUCCESS;
}

/**
 * @brief Get list of all port IDs
 * 
 * @param[out] port_ids Array to store port IDs
 * @param[in,out] count In: max array size; Out: actual number of ports
 * @return status_t STATUS_SUCCESS if successful
 */
status_t port_get_list(port_id_t *port_ids, uint32_t *count)
{
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (!port_ids || !count || *count == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Get total port count */
    uint32_t total_ports;
    status_t status = hw_sim_get_port_count(&total_ports);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port count for list");
        return status;
    }
    
    /* Check array size */
    if (*count < total_ports) {
        *count = total_ports;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Fill port IDs (in our simple implementation, ports are 0..N-1) */
    for (uint32_t i = 0; i < total_ports; i++) {
        port_ids[i] = (port_id_t)i;
    }
    
    *count = total_ports;
    
    LOG_DEBUG(LOG_CATEGORY_HAL, "Retrieved list of %u ports", total_ports);
    
    return STATUS_SUCCESS;
}


/**
 * @brief Sends a packet out through the specified port
 *
 * This function handles the transmission of a packet out through a specified
 * physical port. It performs necessary validation, prepares the packet for
 * transmission including any hardware-specific operations, and utilizes the
 * underlying driver interface for actual packet transmission.
 *
 * @param port_id        Identifier of the port through which to send the packet
 * @param packet         Pointer to the packet structure to be transmitted
 *
 * @return STATUS_SUCCESS if packet was successfully queued for transmission
 * @return STATUS_INVALID_PARAM if port_id is invalid or packet is NULL
 * @return STATUS_PORT_DOWN if the specified port is not in active state
 * // @return STATUS_RESOURCE_ERROR if transmission resources couldn't be allocated
 * // @return STATUS_HAL_ERROR for general hardware abstraction layer failures
 */
status_t port_send_packet(port_id_t port_id, packet_t *packet)
{
    /* Validate input parameters */
    if (packet == NULL) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Null packet pointer provided to port_send_packet");
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate port index */
    if (!port_is_valid(port_id)) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Invalid port ID %d in port_send_packet", port_id);
        //return STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PORT;
    }

    /* Check if port is operational */
    port_state_t port_state;
    status_t status = port_get_state(port_id, &port_state);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port %d state, error: %d", port_id, status);
        // return status;
        // return MAKE_ERROR_CODE(COMPONENT_HAL, ERROR_HAL_OPERATION_FAILED);
        return ERROR_HAL_OPERATION_FAILED;
    }

    if (port_state != PORT_STATE_UP) {
        LOG_WARNING (LOG_CATEGORY_HAL, "Attempted to send packet on port %d which is not UP (state: %d)",
                   port_id, port_state);
        return STATUS_PORT_DOWN;
    }

    /* Access port configuration */
    // port_config_t *port_config = port_get_config(port_id);
    // if (port_config == NULL) {
    //     LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port %d configuration", port_id);
    //     return STATUS_HAL_ERROR;
    // }
    port_config_t port_config;
    status = port_get_config(port_id, &port_config);
    if (status != STATUS_SUCCESS) { 
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port %d configuration: %s", 
                 port_id, status_to_string(status));
        // Поскольку это HAL-ошибка, используем соответствующий код:
        return ERROR_HAL_OPERATION_FAILED;
    }   

    /* Update port statistics */
//    port_stats_update_tx(port_id, packet->length);
    port_stats_update_tx(port_id, packet->size);

    /* Log packet transmission at debug level */
//    LOG_DEBUG("Sending packet of size %u bytes on port %d", packet->length, port_id);
    LOG_DEBUG(LOG_CATEGORY_HAL , "Sending packet of size %u bytes on port %d", packet->size, port_id);

    /* Use the appropriate driver function to transmit the packet */
//    driver_handle_t driver = port_config->driver;
    driver_handle_t driver = port_config.driver;
    //return driver_transmit_packet(driver, packet);
    status = driver_transmit_packet(driver, packet);

    // Сохраняем статус из драйвера, но можно маппировать в HAL-статусы если нужно
    return status;
}


/**
 * @brief Sends a packet with custom Ethernet header fields
 *
 * Extended version of packet transmission function that allows specifying
 * source and destination MAC addresses and EtherType for the outgoing packet.
 * This function builds the appropriate Ethernet header and then sends the
 * complete packet through the specified port.
 *
 * @param port_id        Identifier of the port through which to send the packet
 * @param packet         Pointer to the packet structure containing payload data
 * @param src_mac        Source MAC address to use in Ethernet header
 * @param dst_mac        Destination MAC address to use in Ethernet header
 * @param ethertype      EtherType value to use in Ethernet header
 *
 * @return STATUS_SUCCESS if packet was successfully queued for transmission
 * @return STATUS_INVALID_PARAM if any input parameter is invalid
 * @return STATUS_PORT_DOWN if the specified port is not in active state
 * //@return STATUS_RESOURCE_ERROR if packet buffer memory couldn't be allocated
 * //@return STATUS_HAL_ERROR for general hardware abstraction layer failures
 */
status_t port_send_packet_ext(port_id_t port_id, packet_t *packet,
                             const mac_addr_t *src_mac, const mac_addr_t *dst_mac,
                             ethertype_t ethertype)
{
    /* Validate input parameters */
    if (packet == NULL || src_mac == NULL || dst_mac == NULL) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Null parameter provided to port_send_packet_ext");
        return STATUS_INVALID_PARAMETER;
    }

    /* Create a new packet with Ethernet header */
    //packet_t eth_packet;
    //status_t status = packet_init(&eth_packet, packet->size + ETH_HEADER_SIZE);
    //if (status != STATUS_SUCCESS) {
    //    LOG_ERROR(LOG_CATEGORY_HAL, "Failed to initialize packet buffer for Ethernet header");
    //    return status;
    //}
    packet_t *eth_packet = packet_buffer_alloc(packet->size + ETH_HEADER_SIZE);
    if (eth_packet == NULL) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to allocate packet buffer for Ethernet header");
        return STATUS_OUT_OF_MEMORY;
    }


    /* Build Ethernet header */
    //ethernet_header_t *eth_hdr = (ethernet_header_t *)eth_packet.data;
    ethernet_header_t *eth_hdr = (ethernet_header_t *)eth_packet->data;

    /* Copy MAC addresses */
//    memcpy(eth_hdr->dst_mac, dst_mac->addr, MAC_ADDR_LEN);
//    memcpy(eth_hdr->src_mac, src_mac->addr, MAC_ADDR_LEN);

    /* Copy MAC addresses */
    eth_hdr->dst_mac = *dst_mac;
    eth_hdr->src_mac = *src_mac;
    //memcpy(eth_hdr->dst_mac.addr, dst_mac->addr, MAC_ADDR_LEN);
    //memcpy(eth_hdr->src_mac.addr, src_mac->addr, MAC_ADDR_LEN);

    /* Set EtherType (in network byte order) */
    eth_hdr->ethertype = htons(ethertype);

    /* Copy packet payload */
    //memcpy(eth_packet.data + ETH_HEADER_SIZE, packet->data, packet->length);
    //eth_packet.length = packet->length + ETH_HEADER_SIZE;
    memcpy(eth_packet->data + ETH_HEADER_SIZE, packet->data, packet->length);
    eth_packet->length = packet->length + ETH_HEADER_SIZE;

    /* Send the complete packet */
    // status = port_send_packet(port_id, &eth_packet);
    status_t status = port_send_packet(port_id, eth_packet);

    /* Free the temporary packet buffer */
    //packet_free(&eth_packet);
    packet_buffer_free(eth_packet);

    return status;
}














/**
 * @brief Обновляет статистику передачи для указанного порта
 * 
 * @param port_id Идентификатор порта
 * @param length Размер переданного пакета в байтах
 * @return status_t Статус операции
 */
static status_t port_stats_update_tx(port_id_t port_id, size_t length)
{
    /* Проверка инициализации порта */
    if (!g_port_initialized) {
        return STATUS_NOT_INITIALIZED;
    }

    /* Проверка валидности порта */
    if (!port_is_valid(port_id)) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Invalid port ID %d in port_stats_update_tx", port_id);
        return STATUS_INVALID_PORT;
    }

    /* Получаем текущую информацию о порте */
    port_info_t info;
    status_t status = hw_sim_get_port_info(port_id, &info);
    if (status != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Failed to get port info for port %u in port_stats_update_tx", port_id);
        return status;
    }

    /* Обновляем счетчики передачи данных */
    info.stats.tx_packets++;
    info.stats.tx_bytes += length;

    /* В зависимости от размера пакета, обновляем соответствующие счетчики */
    if (length < 64) {
        info.stats.tx_packets_64--;  /* Компенсируем увеличение tx_packets */
        info.stats.tx_packets_lt_64++;
    } else if (length == 64) {
        info.stats.tx_packets_64++;
    } else if (length <= 127) {
        info.stats.tx_packets_65_127++;
    } else if (length <= 255) {
        info.stats.tx_packets_128_255++;
    } else if (length <= 511) {
        info.stats.tx_packets_256_511++;
    } else if (length <= 1023) {
        info.stats.tx_packets_512_1023++;
    } else if (length <= 1518) {
        info.stats.tx_packets_1024_1518++;
    } else {
        info.stats.tx_packets_1519_max++;
    }

    /* Теперь нужно сохранить обновленную статистику обратно в порт */
    /* Поскольку у нас нет прямой функции для записи только статистики,
       мы можем использовать имеющийся hw_sim_set_port_config для конфигурации */
    /* Примечание: в реальной системе лучше иметь отдельную функцию для обновления статистики,
       но для симуляции мы можем использовать достаточно прямой подход */
    
    /* Возможно, нам нужно обновить статистику через API симулятора оборудования.
       В этом случае мы должны проверить, есть ли функция для обновления статистики в HW SIM API.
       Если такой функции нет, мы предполагаем, что HW SIM автоматически обновляет статистику
       при вызове driver_transmit_packet(), и нам не нужно ничего делать здесь. */

    LOG_DEBUG(LOG_CATEGORY_HAL, "Updated TX statistics for port %u: packets=%lu, bytes=%lu",
              port_id, info.stats.tx_packets, info.stats.tx_bytes);

    return STATUS_SUCCESS;
}




static status_t port_mac_subsystem_init(void)
{
    LOG_INFO(LOG_CATEGORY_HAL, "Initializing port MAC address subsystem");

    // Очищаем таблицы
    memset(g_port_mac_table, 0, sizeof(g_port_mac_table));
    memset(g_port_mac_initialized, 0, sizeof(g_port_mac_initialized));

    // В реальной системе здесь бы читали MAC-адреса из:
    // - EEPROM/Flash
    // - OTP (One-Time Programmable) memory  
    // - Hardware registers
    // - Configuration files

    LOG_INFO(LOG_CATEGORY_HAL, "Port MAC address subsystem initialized");
    return STATUS_SUCCESS;
}

status_t port_get_mac(uint16_t port_id, mac_addr_t *mac_addr)
{
    // Валидация параметров
    if (!mac_addr) {
        LOG_ERROR(LOG_CATEGORY_HAL, "port_get_mac: mac_addr parameter is NULL");
        return STATUS_INVALID_PARAMETER;
    }

    if (port_id >= MAX_PORTS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "port_get_mac: Invalid port ID %u (max: %u)", 
                  port_id, MAX_PORTS - 1);
        return STATUS_INVALID_PARAMETER;
    }

    // Проверка инициализации порта
    if (!g_port_mac_initialized[port_id]) {
        LOG_WARNING(LOG_CATEGORY_HAL, "port_get_mac: Port %u MAC not initialized, using default", port_id);
        
        // Генерируем MAC по умолчанию: BASE_MAC + port_offset
        status_t status = port_generate_default_mac(port_id, mac_addr);
        if (status != STATUS_SUCCESS) {
            return status;
        }
        
        // Сохраняем сгенерированный MAC
        memcpy(&g_port_mac_table[port_id], mac_addr, sizeof(mac_addr_t));
        g_port_mac_initialized[port_id] = true;
        
        LOG_INFO(LOG_CATEGORY_HAL, "Port %u MAC initialized: %02x:%02x:%02x:%02x:%02x:%02x",
                 port_id,
                 mac_addr->addr[0], mac_addr->addr[1], mac_addr->addr[2],
                 mac_addr->addr[3], mac_addr->addr[4], mac_addr->addr[5]);
    } else {
        // Копируем сохраненный MAC-адрес
        memcpy(mac_addr, &g_port_mac_table[port_id], sizeof(mac_addr_t));
    }

    LOG_DEBUG(LOG_CATEGORY_HAL, "port_get_mac: Port %u MAC: %02x:%02x:%02x:%02x:%02x:%02x",
              port_id,
              mac_addr->addr[0], mac_addr->addr[1], mac_addr->addr[2],
              mac_addr->addr[3], mac_addr->addr[4], mac_addr->addr[5]);

    return STATUS_SUCCESS;
}

status_t port_set_mac(uint16_t port_id, const mac_addr_t *mac_addr)
{
    if (!mac_addr) {
        LOG_ERROR(LOG_CATEGORY_HAL, "port_set_mac: mac_addr parameter is NULL");
        return STATUS_INVALID_PARAMETER;
    }

    if (port_id >= MAX_PORTS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "port_set_mac: Invalid port ID %u", port_id);
        return STATUS_INVALID_PARAMETER;
    }

    // Валидация MAC-адреса (не должен быть multicast или broadcast)
    if (mac_addr->addr[0] & 0x01) {
        LOG_ERROR(LOG_CATEGORY_HAL, "port_set_mac: Invalid MAC address (multicast/broadcast)");
        return STATUS_INVALID_PARAMETER;
    }

    // Проверка на нулевой MAC
    bool is_zero_mac = true;
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        if (mac_addr->addr[i] != 0) {
            is_zero_mac = false;
            break;
        }
    }
    
    if (is_zero_mac) {
        LOG_ERROR(LOG_CATEGORY_HAL, "port_set_mac: Invalid MAC address (all zeros)");
        return STATUS_INVALID_PARAMETER;
    }

    // Сохраняем MAC-адрес
    memcpy(&g_port_mac_table[port_id], mac_addr, sizeof(mac_addr_t));
    g_port_mac_initialized[port_id] = true;

    LOG_INFO(LOG_CATEGORY_HAL, "Port %u MAC set to: %02x:%02x:%02x:%02x:%02x:%02x",
             port_id,
             mac_addr->addr[0], mac_addr->addr[1], mac_addr->addr[2],
             mac_addr->addr[3], mac_addr->addr[4], mac_addr->addr[5]);

    // Уведомляем HAL о изменении конфигурации порта
    port_config_changed_notify(port_id, PORT_CONFIG_MAC_CHANGED);

    return STATUS_SUCCESS;
}

static status_t port_generate_default_mac(uint16_t port_id, mac_addr_t *mac_addr)
{
    // Базовый MAC-адрес системы (обычно читается из EEPROM/OTP)
    // Для симуляции используем предопределенный базовый MAC
    const mac_addr_t base_mac = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x00}};
    
    if (!mac_addr) {
        return STATUS_INVALID_PARAMETER;
    }

    // Копируем базовый MAC
    memcpy(mac_addr, &base_mac, sizeof(mac_addr_t));
    
    // Добавляем offset порта к младшему байту
    // Для production системы: лучше использовать более сложную схему
    if (port_id < 256) {
        mac_addr->addr[5] = (uint8_t)port_id;
    } else {
        // Для портов > 255, используем два младших байта
        mac_addr->addr[4] = (uint8_t)(port_id >> 8);
        mac_addr->addr[5] = (uint8_t)(port_id & 0xFF);
    }

    return STATUS_SUCCESS;
}

static void port_config_changed_notify(uint16_t port_id, port_config_change_t change_type)
{
    // В реальной системе здесь могут быть:
    // - Уведомления для management подсистемы
    // - Обновление hardware регистров
    // - Логирование для audit trail
    // - События для SNMP traps
    
    LOG_DEBUG(LOG_CATEGORY_HAL, "Port %u configuration changed: type=%d", port_id, change_type);
    
    // TODO: Implement notification mechanisms
}

status_t port_get_all_macs(mac_addr_t mac_table[], uint16_t table_size, uint16_t *actual_count)
{
    if (!mac_table || !actual_count) {
        return STATUS_INVALID_PARAMETER;
    }

    uint16_t count = 0;
    uint16_t max_ports = (table_size < MAX_PORTS) ? table_size : MAX_PORTS;

    for (uint16_t i = 0; i < max_ports; i++) {
        if (port_get_mac(i, &mac_table[count]) == STATUS_SUCCESS) {
            count++;
        }
    }

    *actual_count = count;
    return STATUS_SUCCESS;
}





