#ifndef SWITCH_SIMULATOR_DRIVER_H
#define SWITCH_SIMULATOR_DRIVER_H

//SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

////#include "common/types.h"           // project-wide basic types
////#include "common/error_codes.h"    // status_t
////#include "hal/packet.h"            // packet_t

#include "../common/types.h"           // project-wide basic types
#include "../common/error_codes.h"    // status_t
#include "packet.h"            // packet_t

/**
 * @file driver.h
 * @brief Generic driver abstraction layer for switch simulator
 *
 * This header defines the base driver interface used by all hardware
 * and simulation drivers in the switch SDK. It uses an inheritance-like
 * pattern by embedding the base struct as the first member of driver
 * implementations.
 */

/* Forward declaration of base driver struct */
struct driver;
typedef struct driver driver_t;

/**
 * @brief Driver type enumeration
 *
 * Identifies the specific type of driver for debugging and
 * runtime type checking purposes.
 */
typedef enum {
    DRIVER_TYPE_ETHERNET_PHY = 1,  /**< Physical Ethernet PHY driver */
    DRIVER_TYPE_SWITCH_ASIC  = 2,  /**< Switch ASIC hardware driver */
    DRIVER_TYPE_SIMULATOR    = 3,  /**< Software simulation driver */
    DRIVER_TYPE_LOOPBACK     = 4,  /**< Loopback test driver */
    DRIVER_TYPE_VIRTUAL      = 5,  /**< Virtual interface driver */
    DRIVER_TYPE_MAX
} driver_type_t;

/**
 * @brief Driver operations table (virtual function table)
 *
 * Each concrete driver implementation must provide these function
 * pointers to implement mandatory and optional driver methods.
 */
typedef struct driver_ops {
    /** Initialize the driver instance */
    status_t (*init)(driver_t *drv);
    /** Transmit a packet through this driver */
    status_t (*transmit)(driver_t *drv, packet_t *pkt);
    /** Shutdown and cleanup the driver */
    status_t (*shutdown)(driver_t *drv);
    /** Optional: Reset the driver to its initial state */
    status_t (*reset)(driver_t *drv);
    /** Optional: Get driver-specific statistics */
    status_t (*get_stats)(driver_t *drv, void *stats);
    /** Optional: Set driver-specific configuration */
    status_t (*set_config)(driver_t *drv, const void *config);
} driver_ops_t;

/**
 * @brief Base driver structure (like a "class" in OOP)
 *
 * All concrete drivers must embed this as the first member.
 * This enables inheritance-like behavior in C.
 */
typedef struct driver {
    const driver_ops_t *ops;      /**< Pointer to driver operations table */
    driver_type_t       drv_type; /**< Driver type identifier */
    uint32_t            flags;    /**< Capability flags */
    char                name[32]; /**< Human-readable name */
} driver_t;

/**
 * @brief Opaque handle to a driver instance
 *
 * Used by BSP, HAL, and application layers. They operate
 * only on this handle without knowing implementation details.
 */
typedef driver_t *driver_handle_t;

/**
 * @brief Driver capability flags
 *
 * Indicates which features the driver supports.
 */
typedef enum {
    DRIVER_FLAG_TX_CAPABLE   = (1 << 0), /**< Supports packet transmission */
    DRIVER_FLAG_RX_CAPABLE   = (1 << 1), /**< Supports packet reception    */
    DRIVER_FLAG_HW_OFFLOAD   = (1 << 2), /**< Hardware acceleration support*/
    DRIVER_FLAG_DMA_CAPABLE  = (1 << 3), /**< DMA transfer support         */
    DRIVER_FLAG_IRQ_CAPABLE  = (1 << 4), /**< Interrupt handling support   */
    DRIVER_FLAG_LOOPBACK     = (1 << 5), /**< Loopback mode support        */
    DRIVER_FLAG_FLOW_CONTROL = (1 << 6)  /**< Flow control support         */
} driver_flags_t;

/* Driver management API via inline wrappers */
static inline status_t driver_init(driver_handle_t h) {
    return h && h->ops && h->ops->init ? h->ops->init(h) : STATUS_INVALID_PARAMETER;
}
static inline status_t driver_transmit_packet(driver_handle_t h, packet_t *p) {
    return h && h->ops && h->ops->transmit && p ? h->ops->transmit(h, p) : STATUS_INVALID_PARAMETER;
}
static inline status_t driver_shutdown(driver_handle_t h) {
    return h && h->ops && h->ops->shutdown ? h->ops->shutdown(h) : STATUS_INVALID_PARAMETER;
}
static inline status_t driver_reset(driver_handle_t h) {
    return h && h->ops ? (h->ops->reset ? h->ops->reset(h) : STATUS_NOT_SUPPORTED)
                       : STATUS_INVALID_PARAMETER;
}
static inline status_t driver_get_stats(driver_handle_t h, void *s) {
    return h && h->ops && s ? (h->ops->get_stats ? h->ops->get_stats(h, s) : STATUS_NOT_SUPPORTED)
                            : STATUS_INVALID_PARAMETER;
}
static inline status_t driver_set_config(driver_handle_t h, const void *c) {
    return h && h->ops && c ? (h->ops->set_config ? h->ops->set_config(h, c) : STATUS_NOT_SUPPORTED)
                             : STATUS_INVALID_PARAMETER;
}
static inline bool driver_has_capability(driver_handle_t h, driver_flags_t f) {
    return h ? ((h->flags & f) != 0) : false;
}

/**
 * @brief Get driver type from handle
 *
 * Returns the driver_type of the given handle, or DRIVER_TYPE_MAX on error.
 */
static inline driver_type_t driver_get_type(driver_handle_t h) {
    return h ? h->drv_type : DRIVER_TYPE_MAX;
}

#endif // SWITCH_SIMULATOR_DRIVER_H





///#ifndef DRIVER_H
///#define DRIVER_H
///
///#include <stdint.h>
///#include "common/error_codes.h"   // status_t
///#include "hal/packet.h"           // packet_t
///
///// forward-declaration
///struct driver;
///
////**
/// * Таблица операций драйвера — каждый драйвер
/// * «подписывается» на этот интерфейс.
/// */
///typedef struct driver_ops {
///    status_t (*init    )(struct driver *drv);
///    status_t (*transmit)(struct driver *drv, packet_t *pkt);
///    status_t (*shutdown)(struct driver *drv);
///    // в будущем при необходимости можно дополнять
///    // status_t (*reset)(struct driver *drv);
///} driver_ops_t;
///
////**
/// * Базовая структура-«класс» драйвера.
/// * concrete drivers embed this as first member.
/// */
///typedef struct driver {
///    const driver_ops_t *ops;
///    uint32_t            drv_type;  ///< код типа драйвера (для отладки)
///} driver_t;
///
////**
/// * opaque handle для BSP / HAL / тестов.
/// * Они оперируют только этим указателем,
/// * не зная деталей реализации.
/// */
///typedef driver_t *driver_handle_t;
///
///
///#endif // DRIVER_H
///
