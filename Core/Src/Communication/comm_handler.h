#ifndef COMM_HANDLER_H
#define COMM_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "protocol.h"

//Innitialize communication
void comm_init(void);
//receive data from USB CDC
void comm_receive_data(uint8_t *data, uint32_t len);
//process received frames(call in main loop)
void comm_process_frames(void);
//send response frame via USB CDC
void comm_send_response(uint8_t *data, uint16_t len);
//check if data is ready for processing
uint8_t comm_data_ready(void);
//test connect
void comm_set_usb_connected(uint8_t connected);
#ifdef __cplusplus
}
#endif

#endif