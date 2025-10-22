#include "pump_control.h"
#include "comm_handler.h"
#include "protocol.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>

/* ============================ */
/* GPIO PIN DEFINITIONS*/
/*=============================*/
#define MOTOR_GPIO_PORT GPIOB
#define MOTOR1_PIN GPIO_PIN_7
#define MOTOR2_PIN GPIO_PIN_8
#define MOTOR3_PIN GPIO_PIN_9

static PumpMotorState_t motors[NUM_PUMPS];
static uint8_t system_state = SYS_STATE_STANDBY;
static uint8_t error_code = ERR_NONE;

static PumpMotorConfig_t motor_config[NUM_PUMPS] = {
    {3.0f, MOTOR_GPIO_PORT, MOTOR1_PIN},
    {8.0f, MOTOR_GPIO_PORT, MOTOR2_PIN},
    {12.0f, MOTOR_GPIO_PORT, MOTOR3_PIN}};

/* Hardware control */
static void motor_hardware_on(uint8_t motor_id) {
  if (motor_id < 1 || motor_id > NUM_PUMPS)
    return;
  const PumpMotorConfig_t *cfg =
      &motor_config[motor_id - 1]; // motorid - 1 theo gia tri cua array
  HAL_GPIO_WritePin(cfg->port, cfg->pin, GPIO_PIN_SET);
}

static void motor_hardware_off(uint8_t motor_id) {
  if (motor_id < 1 || motor_id > NUM_PUMPS)
    return;
  const PumpMotorConfig_t *cfg =
      &motor_config[motor_id - 1]; // motorid - 1 theo gia tri cua array
  HAL_GPIO_WritePin(cfg->port, cfg->pin, GPIO_PIN_RESET);
}

static uint32_t calculate_duration_ms(uint8_t motor_id, uint16_t volume_ml) {
  if (motor_id < 1 || motor_id > NUM_PUMPS)
    return 0;
  const PumpMotorConfig_t *cfg = &motor_config[motor_id - 1];
  float duration_sec = (cfg->sec_per_100ml * volume_ml) / 100.0f;
  return (uint32_t)(duration_sec * 1000.0f);
}

void pump_init(void) {
  for (int i = 0; i < NUM_PUMPS; i++) {
    motors[i].running = 0;
    motors[i].motor_id = i + 1;
    motors[i].start_time = 0;
    motors[i].duration_ms = 0;
    motors[i].target_volume = 0;
    motor_hardware_off(i + 1);
  }
  system_state = SYS_STATE_STANDBY;
  error_code = ERR_NONE;
}

/* TASK - check timeouts*/
void pump_task(void) {
  uint32_t current_time = HAL_GetTick();
  uint8_t any_running = 0;

  for (int i = 0; i < NUM_PUMPS; i++) {
    if (motors[i].running) {
      any_running = 1;
      uint32_t elapsed = current_time - motors[i].start_time;

      if (elapsed >= motors[i].duration_ms) {
        motor_hardware_off(i + 1);
        motors[i].running = 0;
      }
    }
  }
  system_state = any_running ? SYS_STATE_RUNNING : SYS_STATE_STANDBY;
}

/* start motor*/
uint8_t pump_start_motor(uint8_t motor_id, uint8_t volume_ml) {
  if (motor_id < 1 || motor_id > NUM_PUMPS) {
    error_code = ERR_INVALID_PARAM;
    return 0;
  }

  // if (volume_ml < MIN_VOLUME_ML || volume_ml > MAX_VOLUME_ML) {
  //   error_code = ERR_INVALID_PARAM;
  //   return 0;
  // }

  int idx = motor_id - 1;
  if (motors[idx].running) {
    error_code = ERR_SYSTEM_BUSY;
    return 0;
  }

  uint32_t duration = calculate_duration_ms(motor_id, volume_ml);
  if (duration == 0) {
    error_code = ERR_INVALID_PARAM;
    return 0;
  }

  motors[idx].running = 1;
  motors[idx].start_time = HAL_GetTick();
  motors[idx].duration_ms = duration;
  motors[idx].target_volume = volume_ml;

  motor_hardware_on(motor_id);
  error_code = ERR_NONE;

  return 1;
}

/* Stop motor */
void pump_stop_motor(uint8_t motor_id) {
  if (motor_id < 1 || motor_id > NUM_PUMPS)
    return;
  int idx = motor_id - 1;
  motors[idx].running = 0;
  motor_hardware_off(motor_id);
}

/* Stop all*/
void pump_stop_all(void) {
  for (int i = 1; i <= NUM_PUMPS; i++) {
    pump_stop_motor(i);
  }

  error_code = ERR_NONE;
}

/* Get status */
void pump_get_status(uint8_t *status_arry, uint8_t *sys_state,
                     uint8_t *err_code) {
  
  for (int i = 0; i < NUM_PUMPS; i++) {
      status_arry[i] = motors[i].running ? 1 : 0;
  }
  *sys_state = system_state;
  *err_code = error_code;
}

/* Calibration */
uint8_t pump_set_calibration(uint8_t motor_id, float sec_per_100ml) {
  if (motor_id < 1 || motor_id > NUM_PUMPS)
    return 0;
  if (sec_per_100ml <= 0 || sec_per_100ml > 60.0f)
    return 0;
  motor_config[motor_id - 1].sec_per_100ml = sec_per_100ml;
  return 1;
}

uint8_t pump_get_calibration(uint8_t motor_id, float *sec_per_100ml) {
  if (motor_id < 1 || motor_id > NUM_PUMPS)
    return 0;
  *sec_per_100ml = motor_config[motor_id - 1].sec_per_100ml;
  return 1;
}

/* Command handler*/
void pump_handle_command(uint8_t *frame, uint16_t len) {
  uint8_t cmd = frame[0];
  uint8_t inst = frame[2];
  uint8_t response[16];
  uint16_t resp_len;

  switch (cmd) {
  case CMD_PUMP_STATUS:
    if (inst == INST_QUERY) {
      uint8_t status[NUM_PUMPS];
      uint8_t state, err;
      pump_get_status(status, &state, &err);
      
      //build data dynamically
      uint8_t data[NUM_PUMPS + 2]; //automatic sizing 

      //copy status
      for(int i = 0; i < NUM_PUMPS; i++) {
        data[i] = status[i];
      }
      // them system state va error
      data[NUM_PUMPS] = state;
      data[NUM_PUMPS + 1] = err;
      resp_len = protocol_build_frame(CMD_PUMP_STATUS, INST_QUERY, 
                                      data, NUM_PUMPS + 2, response);
      comm_send_response(response, resp_len);
      }
    break;

  case CMD_PUMP_PARAM:
    if (inst == INST_QUERY) {
      uint8_t data[NUM_PUMPS * 2];
      for (int i = 0; i < NUM_PUMPS; i++) {
        uint16_t cal = (uint16_t)(motor_config[i].sec_per_100ml * 10);
        data[i * 2] = (cal >> 8) & 0xFF;
        data[i * 2 + 1] = cal & 0xFF;
      }
      resp_len =
          protocol_build_frame(CMD_PUMP_PARAM, INST_QUERY, data, NUM_PUMPS * 2, response);
      comm_send_response(response, resp_len);
    } else if (inst == INST_SET && len >=(3 + NUM_PUMPS * 2 + 2)) {
      for (int i = 0; i < NUM_PUMPS; i++) {
        uint16_t cal = (frame[3 + i * 2] << 8) | frame[4 + i * 2];
        motor_config[i].sec_per_100ml = cal / 10.0f;
      }
      uint8_t data[1] = {RESP_SUCCESS};
      resp_len =
          protocol_build_frame(CMD_PUMP_PARAM, INST_SET, data, 1, response);
      comm_send_response(response, resp_len);
    }
    break;

  case CMD_PUMP_DISPENSE: /* 0x12 */
    if (inst == INST_SET && len >= 7) {
      uint8_t motor_id = frame[3];
      uint8_t volume = frame[4];
      uint8_t success = pump_start_motor(motor_id, volume);
      uint8_t data[2] = {motor_id, success ? RESP_SUCCESS : RESP_FAILED};
      resp_len =
          protocol_build_frame(CMD_PUMP_DISPENSE, INST_SET, data, 2, response);
      comm_send_response(response, resp_len);
    }
    break;
  }
}