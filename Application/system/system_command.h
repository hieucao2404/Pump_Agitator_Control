#ifndef SYSTEM_COMMANDS_H
#define SYSTEM_COMMANDS_H

#ifdef __cplusplus

extern "C" {
#endif

#include "main.h"
#include <stdint.h>


/* Innitialize system commands module*/
void system_commands_init(void);
/* Handle system commnad frame*/
void system_handle_command(uint8_t *frame, uint16_t len);

/* Emergency stop all mortors*/
void system_emergency_stop(void);

/* Get unified system status*/
void system_get_status(uint8_t *pump_status, uint8_t *agitator_status,
                       uint8_t *system_state, uint8_t *error_code);

/* Reset/initialize all subsystems*/
void system_reset(void);

#ifdef __cpluscplus
}
#endif

#endif /* SYSTEM_COMMANDS_H */