#include "system_command.h"
#include "agitator_control.h"
#include "comm_handler.h"
#include "protocol.h"
#include "pump_control.h"
#include <stdint.h>
#include <string.h>
static uint8_t system_initialized = 0;

void system_commands_init(void) { system_initialized = 1; }

void system_emergency_stop(void) {
  pump_stop_all();
  agitator_stop();
}

void system_handle_unified_status(uint8_t *frame, uint16_t len) {
  uint8_t pump_status[3];
  uint8_t pump_state, pump_err;
  uint8_t agit_running, agit_state, agit_err;

  // Get status from all subsystems
  pump_get_status(pump_status, &pump_state, &pump_err);

  agitator_get_status(&agit_running, &agit_state, &agit_err);

  // Determine overall system state
  uint8_t system_state;
  if (pump_state == SYS_STATE_RUNNING || agit_state == SYS_STATE_RUNNING) {
    system_state = SYS_STATE_RUNNING;
  } else if (pump_state == SYS_STATE_FAULT || agit_state == SYS_STATE_FAULT) {
    system_state = SYS_STATE_FAULT;
  } else {
    system_state = SYS_STATE_STANDBY;
  }

  // Use first error encountered
  uint8_t error_code = (pump_err != ERR_NONE) ? pump_err : agit_err;

  // Build response: M1, M2, M3 , ... , Mn, Agitator, State, Error
  uint8_t data[NUM_PUMPS + 3];
  for (int i = 0; i < NUM_PUMPS; i++) {
    data[i] = pump_status[i];
  }
  data[NUM_PUMPS] = agit_running;
  data[NUM_PUMPS + 1] = system_state;
  data[NUM_PUMPS + 2] = error_code;

  uint8_t response[16];
  uint16_t resp_len =
      protocol_build_frame(CMD_QUERY_STATUS, DEVICE_ALL, data, NUM_PUMPS
        + 3, response);
  comm_send_response(response, resp_len);
}

void system_reset(void) {
  system_emergency_stop();
  pump_init();
  agitator_init();
  system_initialized = 1;
}

/* NEW: Handle system reset command (replaces old CMD_SYSTEM_RESET 0x31) */
void system_handle_command(uint8_t *frame, uint16_t len) {
  uint8_t cmd = frame[0];
  uint8_t device = frame[2];
  uint8_t operation = (len > 3) ? frame[3] : 0;

  // Handle system reset: CMD_QUERY_SET_PARAM + DEVICE_SYSTEM + OP_RESET
  if (cmd == CMD_QUERY_SET_PARAM && device == DEVICE_SYSTEM &&
      operation == OP_RESET) {
    system_reset();

    // Send success response
    uint8_t response[8];
    uint8_t data[1] = {RESP_SUCCESS};
    uint16_t resp_len = protocol_build_frame(CMD_QUERY_SET_PARAM, DEVICE_SYSTEM,
                                             data, 1, response);
    comm_send_response(response, resp_len);
  }
}