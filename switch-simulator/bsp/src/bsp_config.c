/**
 * @file bsp_config.c
 * @brief Implementation of BSP configuration functions
 */

#include "../include/bsp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Default configurations for different board types
static const bsp_config_t default_configs[] = {
    // Generic board
    {
        .board_type = BSP_BOARD_TYPE_GENERIC,
        .num_ports = 8,
        .cpu_frequency_mhz = 800,
        .memory_size_mb = 512,
        .packet_buffer_mb = 32,
        .has_layer3_support = true,
        .has_qos_support = true,
        .has_acl_support = true,
        .has_vxlan_support = false,
        .has_sai_support = true,
        .board_name = "Generic Switch",
        .firmware_version = BSP_VERSION_STRING
    },
    // Small board
    {
        .board_type = BSP_BOARD_TYPE_SMALL,
        .num_ports = 8,
        .cpu_frequency_mhz = 400,
        .memory_size_mb = 256,
        .packet_buffer_mb = 16,
        .has_layer3_support = false,
        .has_qos_support = true,
        .has_acl_support = false,
        .has_vxlan_support = false,
        .has_sai_support = true,
        .board_name = "Small Switch",
        .firmware_version = BSP_VERSION_STRING
    },
    // Medium board
    {
        .board_type = BSP_BOARD_TYPE_MEDIUM,
        .num_ports = 24,
        .cpu_frequency_mhz = 800,
        .memory_size_mb = 512,
        .packet_buffer_mb = 64,
        .has_layer3_support = true,
        .has_qos_support = true,
        .has_acl_support = true,
        .has_vxlan_support = true,
        .has_sai_support = true,
        .board_name = "Medium Switch",
        .firmware_version = BSP_VERSION_STRING
    },
    // Large board
    {
        .board_type = BSP_BOARD_TYPE_LARGE,
        .num_ports = 48,
        .cpu_frequency_mhz = 1200,
        .memory_size_mb = 1024,
        .packet_buffer_mb = 128,
        .has_layer3_support = true,
        .has_qos_support = true,
        .has_acl_support = true,
        .has_vxlan_support = true,
        .has_sai_support = true,
        .board_name = "Large Switch",
        .firmware_version = BSP_VERSION_STRING
    },
    // Datacenter board
    {
        .board_type = BSP_BOARD_TYPE_DATACENTER,
        .num_ports = 64,
        .cpu_frequency_mhz = 2000,
        .memory_size_mb = 4096,
        .packet_buffer_mb = 512,
        .has_layer3_support = true,
        .has_qos_support = true,
        .has_acl_support = true,
        .has_vxlan_support = true,
        .has_sai_support = true,
        .board_name = "Datacenter Switch",
        .firmware_version = BSP_VERSION_STRING
    },
    // Enterprise board (новый тип)
    {
        .board_type = BSP_BOARD_TYPE_ENTERPRISE,
        .num_ports = 32,
        .cpu_frequency_mhz = 1600,
        .memory_size_mb = 2048,
        .packet_buffer_mb = 256,
        .has_layer3_support = true,
        .has_qos_support = true,
        .has_acl_support = true,
        .has_vxlan_support = true,
        .has_sai_support = true,
        .board_name = "Enterprise Switch",
        .firmware_version = BSP_VERSION_STRING
    }
};

// Current active configuration
static bsp_config_t active_config;
static bool is_config_initialized = false;

/**
 * @brief Get default configuration for a board type
 */
static bsp_error_t get_default_config(bsp_board_type_t board_type, bsp_config_t* config) {
    if (config == NULL) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    for (size_t i = 0; i < sizeof(default_configs) / sizeof(default_configs[0]); i++) {
        if (default_configs[i].board_type == board_type) {
            memcpy(config, &default_configs[i], sizeof(bsp_config_t));
            return BSP_SUCCESS;
        }
    }
    
    return BSP_ERROR_INVALID_PARAM;
}

/**
 * @brief Validate configuration parameters
 */
static bsp_error_t validate_config(const bsp_config_t* config) {
    if (config == NULL) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    // Check board type
    if (config->board_type < BSP_BOARD_TYPE_GENERIC || 
        config->board_type > BSP_BOARD_TYPE_DATACENTER) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    // Check port count
    if (config->num_ports == 0 || config->num_ports > 128) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    // Other validations as needed
    // Проверяем packet buffer
    if (config->packet_buffer_mb == 0) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    return BSP_SUCCESS;
}

/**
 * @brief Get default configuration for a specified board type
 * @param board_type Type of board to get configuration for
 * @param config Pointer to store the configuration
 * @return bsp_error_t BSP_SUCCESS if successful, error code otherwise
 */
bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config) {
    return get_default_config(board_type, config);
}


//// ===========================================================================
//// These function I ceate when falling in complile errors for initialize bsp_init() 
//// in static status_t initialize_simulator(void) in main.c file
///**
// * @brief Get default configuration for a specified board type
// * @param board_type Type of board to get configuration for
// * @param config Pointer to store the configuration
// * @return bsp_error_t BSP_SUCCESS if successful, error code otherwise
// */
//bsp_error_t bsp_get_default_config(bsp_board_type_t board_type, bsp_config_t* config) {
//    return get_default_config(board_type, config);
//}
//
///**
// * @brief Set board configuration to default values for specified board type
// * @param board_type Type of board to configure
// * @return bsp_error_t BSP_SUCCESS if successful, error code otherwise
// */
//bsp_error_t bsp_set_default_config(bsp_board_type_t board_type) {
//    bsp_config_t default_config;
//    bsp_error_t result;
//    
//    // Получаем конфигурацию по умолчанию для указанного типа платы
//    result = get_default_config(board_type, &default_config);
//    if (result != BSP_SUCCESS) {
//        return result;
//    }
//    
//    // Применяем полученную конфигурацию
//    return bsp_set_config(&default_config);
//}
//
//// =====================================================================================


/**
 * @brief Set the board configuration
 */
bsp_error_t bsp_set_config(const bsp_config_t* config) {
    bsp_error_t result = validate_config(config);
    if (result != BSP_SUCCESS) {
        return result;
    }
    
    // Apply the configuration
    memcpy(&active_config, config, sizeof(bsp_config_t));
    is_config_initialized = true;
  
    /*
     *
     * Логирование:
     * В функции bsp_set_config() используется printf() для вывода информации. 
     * В промышленном коде лучше использовать специализированный механизм логирования.
     * 
     */
    printf("BSP: Board configured as %s with %u ports\n", 
           active_config.board_name, active_config.num_ports);
    
    return BSP_SUCCESS;
}

/**
 * @brief Initialize default configuration based on board type
 */
bsp_error_t bsp_init_default_config(bsp_board_type_t board_type) {
    bsp_config_t config;
    bsp_error_t result = get_default_config(board_type, &config);
    if (result != BSP_SUCCESS) {
        return result;
    }
    
    return bsp_set_config(&config);
}

/**
 * @brief Get the current board configuration
 */
bsp_error_t bsp_get_config(bsp_config_t* config) {
    if (config == NULL) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    if (!is_config_initialized) {
        return BSP_ERROR_NOT_INITIALIZED;
    }
    
    memcpy(config, &active_config, sizeof(bsp_config_t));
    return BSP_SUCCESS;
}

/**
 * @brief Check if board configuration is initialized
 */
bool bsp_is_config_initialized(void) {
    return is_config_initialized;
}

/**
 * @brief Override specific configuration parameters
 */
bsp_error_t bsp_override_config_param(const char* param_name, const char* param_value) {
  
  /** Нужно проверить:
   *
   * Возможная утечка памяти:
   * В функции bsp_override_config_param() при установке board_name 
   * используется strdup(), но нет кода для освобождения предыдущего значения.
   * Это может привести к утечке памяти при многократном изменении имени платы.
   *
   *
   */

    if (!is_config_initialized) {
        return BSP_ERROR_NOT_INITIALIZED;
    }
    
    if (param_name == NULL || param_value == NULL) {
        return BSP_ERROR_INVALID_PARAM;
    }
    
    // Handle various parameter overrides
    if (strcmp(param_name, "num_ports") == 0) {
        int ports = atoi(param_value);
        if (ports <= 0 || ports > 128) {
            return BSP_ERROR_INVALID_PARAM;
        }
        active_config.num_ports = (uint32_t)ports;
    } else if (strcmp(param_name, "board_name") == 0) {
        // Allocate memory for the new name (this is simplified)
        // In a real implementation, handle memory properly
        active_config.board_name = strdup(param_value);
    } else if (strcmp(param_name, "has_layer3_support") == 0) {
        active_config.has_layer3_support = (strcmp(param_value, "true") == 0);
    } else if (strcmp(param_name, "has_qos_support") == 0) {
        active_config.has_qos_support = (strcmp(param_value, "true") == 0);
    } else if (strcmp(param_name, "has_acl_support") == 0) {
        active_config.has_acl_support = (strcmp(param_value, "true") == 0);
    } else if (strcmp(param_name, "cpu_frequency_mhz") == 0) {
        int freq = atoi(param_value);
        if (freq <= 0) {
            return BSP_ERROR_INVALID_PARAM;
        }
        active_config.cpu_frequency_mhz = (uint32_t)freq;
    } else if (strcmp(param_name, "memory_size_mb") == 0) {
        int mem = atoi(param_value);
        if (mem <= 0) {
            return BSP_ERROR_INVALID_PARAM;
        }
        active_config.memory_size_mb = (uint32_t)mem;
    } else {
        return BSP_ERROR_NOT_SUPPORTED;
    }
    
    return BSP_SUCCESS;
}




// Добавьте в конец файла новые функции:

/**
 * @brief Initialize BSP configuration with default values
 */
bsp_error_t bsp_init_config(bsp_config_t* config, bsp_board_type_t board_type) {
    if (!config) {
        return BSP_ERROR_NULL_POINTER;
    }
    
    // Очищаем структуру
    memset(config, 0, sizeof(bsp_config_t));
    
    // Получаем конфигурацию по умолчанию
    bsp_error_t result = get_default_config(board_type, config);
    if (result != BSP_SUCCESS) {
        return result;
    }
    
    return BSP_SUCCESS;
}

/**
 * @brief Set board name in configuration
 */
bsp_error_t bsp_set_board_name(bsp_config_t* config, const char* name) {
    if (!config || !name) {
        return BSP_ERROR_NULL_POINTER;
    }
    
    // Проверяем длину имени
    size_t name_len = strlen(name);
    if (name_len >= BSP_MAX_BOARD_NAME_LEN) {
        return BSP_ERROR_BUFFER_OVERFLOW;
    }
    
    // Копируем имя в массив
    snprintf(config->board_name, sizeof(config->board_name), "%s", name);
    
    return BSP_SUCCESS;
}


























