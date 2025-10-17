#include "agitator_control.h"
#include "comm_handler.h"
#include "protocol.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>

/* GPIO Configuration*/
#define AGITATOR_PORT GPIOB
#define AGITATOR_PIN GPIO_PIN_6

// Private variables
static AgitatorState_t agitator = {0};
static uint8_t system_state = SYS_STATE_STANDBY;
static uint8_t error_code = ERR_NONE;

// hard ware control fucntion
static void agitator_hardware_on(void) {
  HAL_GPIO_WritePin(AGITATOR_PORT, AGITATOR_PIN, GPIO_PIN_SET);
}

static void agitator_hardware_off(void) {
  HAL_GPIO_WritePin(AGITATOR_PORT, AGITATOR_PIN, GPIO_PIN_RESET);
}

/* Initialize agitator*/
void agitator_init(void) {
  agitator.running = 0;
  agitator.start_requested = 0;
  agitator.start_time = 0;
  agitator.duration_ms = 0;
  agitator.requested_duration = 0;

  // set off before do anything
  agitator_hardware_off();
  system_state = SYS_STATE_STANDBY;
  error_code = ERR_NONE;
}

/* Xu li tac vu cua may khuya*/
void agitator_task(void) {
  uint32_t current_time = HAL_GetTick();

  /* Handle pending start*/
  if (agitator.start_requested) {
    agitator.running = 1;
    agitator.start_time = current_time;
    agitator.duration_ms = agitator.requested_duration;
    agitator.start_requested = 0;
    agitator_hardware_on();
  }

  // Check timeout
  if (agitator.running) {
    uint32_t elapsed = current_time - agitator.start_time;

    if (elapsed >= agitator.duration_ms) {
      agitator_hardware_off();
      agitator.running = 0;
    }
  }
  system_state = agitator.running ? SYS_STATE_RUNNING : SYS_STATE_STANDBY;
}

// start agitator
uint8_t agitator_start(uint32_t duration_ms) {
  // gioi han thoi gian chay cua may khuya (min and max)
   if (duration_ms < MIN_AGIT_DURATION || duration_ms > MAX_AGIT_DURATION) {
        error_code = ERR_INVALID_PARAM; 
        return 0;
    }

  if (agitator.running){
    error_code = ERR_SYSTEM_BUSY;
    return 0;}

  agitator.requested_duration = duration_ms;
  agitator.start_requested = 1;
  error_code = ERR_NONE;

  return 1;
}

/* stop agitator*/
void agitator_stop(void) {
  agitator.running = 0;
  agitator.start_requested = 0;
  agitator_hardware_off();
}

// get status
void agitator_get_status(uint8_t *running, uint8_t *sys_state, uint8_t *error_code_out) {
  *running = agitator.running ? 0x01 : 0x00;
  *sys_state = system_state;
  *error_code_out = error_code;
}

// handle commands
void agitator_handle_command(uint8_t *frame, uint16_t len) {


  uint8_t cmd = frame[0];
  uint8_t inst = frame[2];
  uint8_t response[12];
  uint16_t resp_len;

  switch (cmd) {
  case CMD_AGIT_STATUS: // 0x20
    if (inst == INST_QUERY) {
      uint8_t running, state, err;
      agitator_get_status(&running, &state, &err);

      uint8_t data[3] = {running, state, err};
      resp_len =
          protocol_build_frame(CMD_AGIT_STATUS, INST_QUERY, data, 2, response);
      comm_send_response(response, resp_len);
    }
    break;

  case CMD_AGIT_PARAM: // 0x21 - Reversed ??
    if (inst == INST_QUERY) {
      uint8_t data[1] = {0x00};
      resp_len =
          protocol_build_frame(CMD_AGIT_PARAM, INST_QUERY, data, 1, response);
      comm_send_response(response, resp_len);
    } else if (inst == INST_SET) {
      uint8_t data[1] = {RESP_SUCCESS};
      resp_len =
          protocol_build_frame(CMD_AGIT_PARAM, INST_SET, data, 1, response);
      comm_send_response(response, resp_len);
    }
    break;

  case CMD_AGIT_CONTROL: // 0x22
    if (inst == INST_SET && len >= 7) {
      uint16_t duration = (frame[3] << 8) | frame[4];
      uint8_t success = agitator_start(duration);

      uint8_t data[1] = {success ? RESP_SUCCESS : RESP_FAILED};
      resp_len =
          protocol_build_frame(CMD_AGIT_CONTROL, INST_SET, data, 1, response);
          comm_send_response(response, resp_len);
    }
    break;
  }
}
