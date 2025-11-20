/**
Pump Controll
- PB7: Pump motor 1
- PB8: Pump motor 2
- PB9: Pump motor 3
- PB12: Pump motor 4  <-- NEW
- PB13: Pump motor 5  <-- NEW
- PB14: Pump motor 6  <-- NEW
 */

#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

  /**====================
    CONFIGURATION
    ===================== */

    #define NUM_PUMPS 6

    //** Pump motor state struct */
    typedef struct {
      uint8_t running;
      uint8_t motor_id;
      uint32_t start_time;
      uint32_t duration_ms;
      uint16_t target_volume;
    } PumpMotorState_t;

    //Pump motor configuration
    typedef struct {
      float sec_per_100ml;
      GPIO_TypeDef *port;
      uint16_t pin;
    } PumpMotorConfig_t;

    //Innitialized pump control system
    // goi truoc main loop
    void pump_init(void);
    // Xu lu tac vu
    void pump_task(void);
    // xu ly len nhan vao
    void pump_handle_command(uint8_t *frame, uint16_t len);
    // chay motor duoc chi dinh
    uint8_t pump_start_motor(uint8_t motor_id, uint8_t volume_ml);
    // Stop may bom dc chi dinh
    void pump_stop_motor(uint8_t motor_id);
    // Dung tat ca 
    void pump_stop_all(void);
    // kiem tra trang thai may bom
    void pump_get_status(uint8_t *status_bits, uint8_t *sys_state, uint8_t *err_code);

    //Set dinh luong cho may bom
    uint8_t pump_set_calibration(uint16_t *cal_values);

    // Xac nhan dinh luong cho may bom
    void pump_get_calibration(uint16_t *cal_values);

  #ifdef __cplusplus

  }
#endif
#endif /* PUMP_CONTROL_H */