#include "system_command.h"
#include "agitator_control.h"
#include "comm_handler.h"
#include "protocol.h"
#include "pump_control.h"
#include "system_command.h"
#include <stdint.h>

static uint8_t system_initialized = 0;

void system_commands_init(void) { system_initialized = 1; }

void system_emergency_stop(void) {
  pump_stop_all();
  agitator_stop();
}

void system_get_status(uint8_t *pump_status, uint8_t *agitator_status,
                       uint8_t *system_state, uint8_t *error_code) {
  uint8_t pump_state, pump_err;
  uint8_t agit_state, agit_err;

  pump_get_status(pump_status, &pump_state, &pump_err);
  agitator_get_status(agitator_status, &agit_state, &agit_err);

  if (pump_state == SYS_STATE_RUNNING || agit_state == SYS_STATE_RUNNING) {
    *system_state = SYS_STATE_RUNNING;
  } else if (pump_state == SYS_STATE_FAULT || agit_state == SYS_STATE_FAULT) {
    *system_state = SYS_STATE_FAULT;
  } else {
    *system_state = SYS_STATE_STANDBY;
  }
  *error_code = (pump_err != ERR_NONE) ? pump_err : agit_err;
}

void system_reset(void) {
  system_emergency_stop();
  pump_init();
  agitator_init();
  system_initialized = 1;
}

void system_handle_command(uint8_t *frame, uint16_t len) {
  uint8_t cmd = frame[0];
  uint8_t inst = frame[2];
  uint8_t response[16];
  uint16_t resp_len;

  switch (cmd) {
  case CMD_SYSTEM_STATUS: /* 0x30  */
    if (inst == INST_QUERY) {
      uint8_t pump_status, agit_status, state, err;
      system_get_status(&pump_status, &agit_status, &state, &err);
      uint8_t data[4] = {pump_status, agit_status, state, err};
      resp_len = protocol_build_frame(CMD_SYSTEM_STATUS, INST_QUERY, data, 4,
                                      response);
      comm_send_response(response, resp_len);
    }
    break;

  case CMD_SYSTEM_RESET: /* 0x31 */
    if (inst == INST_SET) {
      system_reset();
      uint8_t data[1] = {RESP_SUCCESS};
      resp_len =
          protocol_build_frame(CMD_SYSTEM_RESET, INST_SET, data, 1, response);
      comm_send_response(response, resp_len);
    }
    break;

  case CMD_EMERGENCY_STOP: /* 0x32 */
    if (inst == INST_SET) {
      system_emergency_stop();
      uint8_t data[1] = {RESP_SUCCESS};
      resp_len =
          protocol_build_frame(CMD_EMERGENCY_STOP, INST_SET, data, 1, response);
      comm_send_response(response, resp_len);
    }
    break;
  }
}