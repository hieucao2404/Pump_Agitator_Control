#ifndef AGITATOR_CONTROL_H
#define AGITATOR_CONTROL_H

#ifdef __cplusplus
extern "C" {
  #endif

  #include <stdint.h>
  #include "main.h"

  /*
  agitator motro sate structure
  */
  typedef struct {
    uint8_t running; //motor dang chay
    uint8_t start_requested; // treo
    uint32_t start_time; // start time
    uint32_t duration_ms;
    uint32_t requested_duration; 
  } AgitatorState_t;


  //Cac ham cho may khuya
  void agitator_init(void);
  void agitator_task(void);
  void agitator_handle_command(uint8_t *frame, uint16_t len);
  uint8_t agitator_start(uint32_t duration_ms);
  void agitator_stop(void);
  void agitator_get_status(uint8_t *running, uint8_t *sys_state);

#ifdef __cplusplus
}
#endif

#endif // AGITATOR_CONTROL_H