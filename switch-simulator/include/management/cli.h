/**
 * @file cli.h
 * @brief Command Line Interface (CLI) for switch management
 *
 * This header defines the interface for the switch simulator's command-line
 * interface, allowing configuration and monitoring of the switch.
 */

#ifndef CLI_H
#define CLI_H
//////SDK_ES_SWITCH_SIMULATOR_INCLUDE_COMMON_CONFIG_H

#include <stddef.h>
#include <stdbool.h>
#include "../common/error_codes.h"
#include "../common/types.h"

typedef status_t (*cli_cmd_handler_t)(int argc, char **argv, char *output, size_t output_len);

typedef struct {
    const char *name;           /**< Command name */
    const char *help;           /**< Help text for the command */
    const char *usage;          /**< Usage example */
    cli_cmd_handler_t handler;  /**< Command handler function */
} cli_command_t;

typedef struct cli_context_s {
    void *private_data;         /**< Private data for CLI implementation */
} cli_context_t;

status_t cli_init(cli_context_t *ctx);
status_t cli_register_command(cli_context_t *ctx, const cli_command_t *cmd);
status_t cli_register_commands(cli_context_t *ctx, const cli_command_t *cmds, size_t count);
status_t cli_execute(cli_context_t *ctx, const char *command_str, 
                         char *output, size_t output_len);
status_t cli_interactive_mode(cli_context_t *ctx);
status_t cli_enable_history(cli_context_t *ctx, size_t history_size);
status_t cli_enable_auto_complete(cli_context_t *ctx, bool enable);
status_t cli_set_prompt(cli_context_t *ctx, const char *prompt);
status_t cli_cleanup(cli_context_t *ctx);

#endif /* CLI_H */
