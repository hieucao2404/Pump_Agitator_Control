#include "comm_handler.h"
#include "agitator_control.h"
#include "protocol.h"
#include "pump_control.h"

 #include "system_command.h"
#include "usbd_cdc_if.h"
#include <stdint.h>
#include <string.h>

#define RX_BUFFER_SIZE 128
#define MAX_FRAME_SIZE 32

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint16_t rx_index = 0;
static volatile uint8_t data_ready = 0;

static uint8_t usb_connected =  0;
static uint32_t usb_connect_tick = 0;

void comm_set_usb_connected(uint8_t connected) {
  usb_connected = connected ? 1 : 0;
  if(connected) {
    usb_connect_tick = HAL_GetTick();
    // optionally flush buffer
    rx_index = 0;
    memset(rx_buffer, 0, RX_BUFFER_SIZE);
  } else {
    usb_connect_tick = 0;
  }
}

/*
Tinh checksum (modulo 256)
 */
uint8_t protocol_calc_checksum(uint8_t *data, uint16_t len) {
  uint32_t sum = 0;
  for (uint16_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(sum & 0xFF);
}

/*
 Xac nhan frame
*/
uint8_t protocol_validate_frame(uint8_t *frame, uint16_t len) {
  if (len < 5)
    return 0; // hex code khong hop le (khong du dai)
  if (frame[len - 1] != FRAME_END)
    return 0; // hex code khong co end

  uint8_t length_code = frame[1];
  if (length_code != len)
    return 0; // kiem tra len nhan duoc khong bi can thiep (make sure dung lenh
              // can thuc hien)

  uint16_t cs_pos = len - 2;
  uint8_t calc_cs = protocol_calc_checksum(frame, cs_pos);
  uint8_t frame_cs = frame[cs_pos];

  return (calc_cs == frame_cs) ? 1 : 0;
}

/*
Cat nho hex code de phan tich
*/
uint16_t protocol_build_frame(uint8_t cmd, uint8_t device, uint8_t *data,
                              uint8_t data_len, uint8_t *out_frame) {
  uint16_t idx = 0;

  out_frame[idx++] = cmd;

  uint8_t total_len =
      3 + data_len +
      2; // length code = tong cua cac phan tu trong string va` data length
  out_frame[idx++] = total_len; // dat ngay sau command code

  // sau do dai tong la cac lenh thuc thi
  out_frame[idx++] = device;

  // luu tru data cho checksum
  for (uint8_t i = 0; i < data_len; i++) {
    out_frame[idx++] = data[i]; //
  }

  uint8_t checksum = protocol_calc_checksum(out_frame, idx);
  // sau lenh thuc thi la checksum
  out_frame[idx++] = checksum;

  // cuoi cung la end frame
  out_frame[idx++] = FRAME_END;

  return idx;
}

/*
 Khai bao xu li giao tie
 */
void comm_init(void) {
  rx_index = 0;
  data_ready = 0;
  memset(rx_buffer, 0, RX_BUFFER_SIZE);
}

/*
Nhan data tu USB CDC
*/
void comm_receive_data(uint8_t *data, uint32_t len) {
     // CDC_Transmit_FS(data, len);
  for (uint32_t i = 0; i < len; i++) {
    // kiem tra do dai cua lenh co qua buffer length khong
    if (rx_index < RX_BUFFER_SIZE) {
      // ngan hon do dai cua frame -> add
      rx_buffer[rx_index++] = data[i];
    } else {
      // neu buffer bi day, shift data nguoc ve 1 byte, day data cu ra khoi
      // buffer
      memmove(rx_buffer, rx_buffer + 1, RX_BUFFER_SIZE - 1);
      rx_buffer[RX_BUFFER_SIZE - 1] = data[i];
    }
  }

  data_ready = 1;
}

/* gui cau tra loi nguoc lai bang usb*/
void comm_send_response(uint8_t *data, uint16_t len) {
  CDC_Transmit_FS(data, len);
}

/*
  xu li frame nhan duoc
*/
void comm_process_frames(void) {
  // edge case, khong san sang nhan lenh
  if (!data_ready || rx_index == 0)
    return;

  data_ready = 0;

  int end_pos = -1;
  for (int i = 0; i < rx_index; i++) {
    if (rx_buffer[i] == FRAME_END) {
      end_pos = i; // tim kiem diem cuoi
      break;
    }
  }

  if (end_pos < 0) {
    if (rx_index >= RX_BUFFER_SIZE - 10) {
      // prevent buffer overflow
      memmove(rx_buffer, rx_buffer + 1, rx_index - 1);
      rx_index--;
    }
    return;
  }

  uint16_t frame_len = end_pos + 1;

  if (!protocol_validate_frame(rx_buffer, frame_len)) {
    memmove(rx_buffer, rx_buffer + 1, rx_index - 1);
    rx_index--;
    data_ready = 1;
    return;
  }

  uint8_t cmd = rx_buffer[0];

  // Check what this command will execute

  uint8_t device_target = rx_buffer[2];

  switch(cmd) {
    case CMD_QUERY_STATUS: //0x10
     if (device_target == DEVICE_PUMP) {
      pump_handle_command(rx_buffer, frame_len);
     } else  if( device_target == DEVICE_AGITATOR) {
      agitator_handle_command(rx_buffer, frame_len); 
     } else if(device_target == DEVICE_ALL) {
      system_handle_unified_status(rx_buffer, frame_len);
     }
     break;
    
    case CMD_QUERY_SET_PARAM:
     if(device_target == DEVICE_PUMP) {
        pump_handle_command(rx_buffer, frame_len);
     }
     break;

     case CMD_START_CONTROL:  // 0x12
    if(device_target == DEVICE_PUMP) {
        pump_handle_command(rx_buffer, frame_len);
    } else if(device_target == DEVICE_AGITATOR) {
        agitator_handle_command(rx_buffer, frame_len);
    }
    break;

    case CMD_EMERGENCY_STOP: // 0x32
     system_emergency_stop();
     //send success response
     uint8_t response[8];
     uint8_t data[1] = {RESP_SUCCESS};
    uint16_t resp_len = protocol_build_frame(CMD_EMERGENCY_STOP, 0, data, 1, response);
      comm_send_response(response, resp_len);
      break;
  }

  //clean up buffer
  memmove(rx_buffer, rx_buffer + frame_len, rx_index - frame_len);
  rx_index -= frame_len;

  if(rx_index > 0) data_ready = 1;
}

/*
check if data ready*/
uint8_t comm_data_ready(void) { return data_ready; }