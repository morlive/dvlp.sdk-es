/**
 * @file main.c
 * @brief Основной файл проекта switch-simulator
 * 
 * Этот файл содержит основную точку входа программы и инициализацию
 * всех компонентов симулятора сетевого коммутатора.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include "common/logging.h"
#include "common/types.h"
#include "common/config.h"
#include "hal/hw_resources.h"
#include "l2/mac_table.h"
#include "l2/vlan.h"
#include "l3/routing_table.h"
#include "management/cli.h"
#include "management/stats.h"
#include "sai/sai_adapter.h"
#include "bsp.h"


/* Глобальные переменные */
static volatile bool g_running = true;

/* Добавить глобальную переменную cli_ctx */
static volatile  cli_context_t    cli_ctx;
static volatile  stats_context_t  stats_ctx;

/**
 * Обработчик сигналов для корректного завершения работы
 */
static void signal_handler(int signum) {
    LOG_INFO(LOG_CATEGORY_SYSTEM, "Получен сигнал %d, завершение работы...", signum);
    g_running = false;
}

/**
 * Инициализация обработчиков сигналов
 */
static status_t setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        LOG_ERROR(LOG_CATEGORY_SYSTEM, "Не удалось установить обработчик для SIGINT");
        return ERROR_INTERNAL;
    }
    
    if (sigaction(SIGTERM, &sa, NULL) != 0) {
        LOG_ERROR(LOG_CATEGORY_SYSTEM, "Не удалось установить обработчик для SIGTERM");
        return ERROR_INTERNAL;
    }
    
    return STATUS_SUCCESS;
}

/**
 * Инициализация всех компонентов симулятора
 */
static status_t initialize_simulator(void) {
    status_t err;
    
    bsp_error_t bsp_err;

    // Инициализация платформы (BSP)
    LOG_INFO(LOG_CATEGORY_BSP, "Инициализация платформы...");

    // Создаем конфигурацию BSP
    bsp_config_t bsp_config;
//    char temp_name[] = "Medium Switch";
    
//    // Используем одну из преднастроенных конфигураций, например, MEDIUM
//    bsp_config.board_type = BSP_BOARD_TYPE_MEDIUM;
//    bsp_config.num_ports = 24;
//    bsp_config.cpu_frequency_mhz = 800;
//    bsp_config.memory_size_mb = 512;
//    bsp_config.has_layer3_support = true;
//    bsp_config.has_qos_support = true;
//    bsp_config.has_acl_support = true;
//    bsp_config.board_name = temp_name;


    // Инициализируем конфигурацию с предустановками
    bsp_err = bsp_init_config(&bsp_config, BSP_BOARD_TYPE_MEDIUM);
    if (bsp_err != BSP_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_BSP, "Ошибка инициализации конфигурации BSP: %d", bsp_err);
        return BSP_ERROR_INIT_FAILED;
    }


    // Можно дополнительно изменить имя платы, если нужно
    bsp_set_board_name(&bsp_config, "Custom Medium Switch");

    
    // Теперь передаем указатель на конфигурацию
    err = bsp_init(&bsp_config);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_BSP, "Ошибка инициализации платформы: %d", err);
        return err;
    }
    
    // Инициализация аппаратных ресурсов (HAL)
    LOG_INFO(LOG_CATEGORY_HAL, "Инициализация аппаратных ресурсов...");
    err = hw_resources_init();
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_HAL, "Ошибка инициализации аппаратных ресурсов: %d", err);
        return err;
    }

    // Создаем конфигурацию для таблицы MAC-адресов
    mac_table_config_t mac_config;

    // Заполняем поля конфигурации в соответствии с определением структуры
    mac_config.learning_enabled = true;        // Включаем функцию обучения MAC-адресам
    mac_config.aging_time = 300;               // Устанавливаем время устаревания (в секундах)
    mac_config.max_entries = 8192;             // Максимальное количество записей в таблице
    mac_config.move_detection = true;          // Включаем обнаружение перемещения MAC-адресов

    // Инициализация L2 компонентов
    LOG_INFO(LOG_CATEGORY_L2, "Инициализация L2 компонентов...");
//    err = mac_table_init(&mac_config);
    err = mac_table_init(mac_config.max_entries, mac_config.aging_time);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_L2, "Ошибка инициализации таблицы MAC-адресов: %d", err);
        return err;
    }
    
    err = vlan_init(bsp_config.num_ports);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_L2, "Ошибка инициализации VLAN: %d", err);
        return err;
    }
    
    // Создаем таблицу маршрутизации и инициализируем её базовыми значениями
    routing_table_t routing_table;

    routing_table.route_count = 0;         // Изначально таблица пуста
    routing_table.last_update_time = 0;    // Время последнего обновления
    routing_table.changed = false;         // Флаг изменения
    
    // Инициализируем таблицу маршрутизации
    // Инициализация L3 компонентов
    LOG_INFO(LOG_CATEGORY_L3, "Инициализация L3 компонентов...");
    err = routing_table_init(&routing_table);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_L3, "Ошибка инициализации таблицы маршрутизации: %d", err);
        return err;
    }
    
    // Создаем и инициализируем контекст оборудования
    hw_context_t hw_context;
    memset(&hw_context, 0, sizeof(hw_context_t)); // Сначала обнуляем всю структуру
    
    // Инициализация полей структуры
    hw_context.hw_registers = NULL;                     // Или выделите память, если это необходимо
    hw_context.port_count = CONFIG_DEFAULT_PORT_COUNT;  // Используйте константу из конфигурации
    hw_context.is_initialized = false;                  // Устанавливаем false, т.к. инициализация еще не завершена
    hw_context.device_handle = NULL;                    // Будет установлено позже драйвером устройства
    hw_context.dma_memory = NULL;                       // Или выделите память для DMA операций
    hw_context.device_id = 0;                           // Или используйте соответствующий ID устройства
    
    // Инициализация мьютекса
    pthread_mutex_init(&hw_context.hw_mutex, NULL);

    // Инициализация SAI
    LOG_INFO(LOG_CATEGORY_SAI, "Инициализация SAI...");
    //err = sai_adapter_init(&hw_context.hw_mutex, NULL);
    err = sai_adapter_init(&hw_context);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_SAI, "Ошибка инициализации SAI адаптера: %d", err);
        return err;
    }
    
    // Создаем переменную контекста статистики и обнуляем её
    memset((void*)&stats_ctx, 0, sizeof(stats_ctx));

    // Инициализация компонентов управления
    LOG_INFO(LOG_CATEGORY_CONTROL, "Инициализация компонентов управления...");
    err = stats_init((void*)&stats_ctx);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_CONTROL, "Ошибка инициализации системы статистики: %d", err);
        return err;
    }

    // Инициализируем глобальную cli_ctx
    memset((void*)&cli_ctx, 0, sizeof(cli_ctx));

    err = cli_init((void*)&cli_ctx);
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_CLI, "Ошибка инициализации CLI: %d", err);
        return err;
    }
    
    LOG_INFO(LOG_CATEGORY_SYSTEM, "Инициализация завершена успешно");
    return STATUS_SUCCESS;
}

/**
 * Деинициализация всех компонентов симулятора
 */
static void deinitialize_simulator(void) {
    LOG_INFO(LOG_CATEGORY_SYSTEM, "Деинициализация системы...");
    
    // Деинициализация в обратном порядке
    cli_cleanup((void*)&cli_ctx);           //    cli_deinit();
    stats_cleanup((void*)&stats_ctx);       //    stats_deinit();
    sai_adapter_deinit();
    routing_table_cleanup();                // routing_table_deinit();
    vlan_deinit();
    mac_table_deinit();
    hw_resources_shutdown();                //    hw_resources_deinit();
    bsp_deinit();
    
    LOG_INFO(LOG_CATEGORY_SYSTEM, "Деинициализация завершена");
}

/**
 * Основной цикл симулятора
 */
static void simulator_main_loop(void) {
    LOG_INFO(LOG_CATEGORY_CONTROL, "Запуск основного цикла симулятора");
    
    while (g_running) {
        // Обработка пакетов и других задач
        // ...
        
        // Чтобы не загружать процессор на 100%
        usleep(1000);
    }
    
    LOG_INFO(LOG_CATEGORY_CONTROL, "Основной цикл симулятора завершен");
}

/**
 * Основная функция программы
 */
int main(int argc, char *argv[]) {
    status_t err;
    
    // Инициализация системы логирования
    // log_init("switch_simulator.log");           //    log_init();
    //   ^
    //    в конечной версии нужно будет его открыть вместо log_init(NULL);      
    // Инициализация системы логирования (вывод в консоль)
    log_init(NULL);

    LOG_INFO(LOG_CATEGORY_SYSTEM, "Switch Simulator запущен");
    
    // Проверка и обработка аргументов командной строки
    // Здесь можно добавить парсинг аргументов
    
    // Настройка обработчиков сигналов
    err = setup_signal_handlers();
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_SYSTEM, "Ошибка при настройке обработчиков сигналов");
        //log_deinit();
        log_shutdown();
        return EXIT_FAILURE;
    }
    
    // Инициализация всех компонентов симулятора
    err = initialize_simulator();
    if (err != STATUS_SUCCESS) {
        LOG_ERROR(LOG_CATEGORY_SYSTEM, "Ошибка при инициализации симулятора");
        // log_deinit();
        log_shutdown();
        return EXIT_FAILURE;
    }
    
    // Запуск основного цикла
    simulator_main_loop();
    
    // Деинициализация
    deinitialize_simulator();
    
    LOG_INFO(LOG_CATEGORY_SYSTEM, "Switch Simulator завершен");
    //log_deinit();
    log_shutdown();
    
    return EXIT_SUCCESS;
}
