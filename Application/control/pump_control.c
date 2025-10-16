#ifndef COMM_HANDLER_H
#define COMM_HANDLER_H

#ifdef __cplusplus
extern "C" {
  #endif

  #include <stdint.h>
  #include "protocol.h"

  //brief innitilized communation handler
  void comm_inin(void);
  // Nhan data tu USB CDC (USB interrupt??)
  void comm_receive_data(uint8_t *data, uint32_t len);
  // Process data nhan duoc (vong lap while cua ham main)
  void comm_process_frames(void);
  // Gui cau tra loi ve bang USB CDC
  void comm_send_response(uint8_t *data, uint16_t len);
  // check if data s ready for processing
  uint8_t comm_data_ready(void);

  #ifdef __cplusplus
}
#endif

#endif 