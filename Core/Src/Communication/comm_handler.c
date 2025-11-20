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
    
  // Check against MAX_FRAME_SIZE
  if (len > MAX_FRAME_SIZE)
    return 0; // Frame too long
    
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

  // 3 bytes = CMD, LEN, DEV
  // 2 bytes = CS, END
  uint8_t total_len = 3 + data_len + 2; 
  
  out_frame[idx++] = total_len; 

  out_frame[idx++] = device;

  for (uint8_t i = 0; i < data_len; i++) {
    out_frame[idx++] = data[i]; 
  }

  uint8_t checksum = protocol_calc_checksum(out_frame, idx);
  out_frame[idx++] = checksum;

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
    if (rx_index < RX_BUFFER_SIZE) {
      rx_buffer[rx_index++] = data[i];
    } else {
      // Buffer is full, shift old data out
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
  if (!data_ready || rx_index == 0)
    return;

  data_ready = 0; // Will be set to 1 again if data remains

  int end_pos = -1;
  for (int i = 0; i < rx_index; i++) {
    if (rx_buffer[i] == FRAME_END) {
      end_pos = i; // tim kiem diem cuoi
      break;
    }
  }

  if (end_pos < 0) {
    // No complete frame found
    if (rx_index >= RX_BUFFER_SIZE) {
      // Buffer is full but no FRAME_END, data is corrupt
      // Clear buffer to prevent overflow
      rx_index = 0; 
    }
    return; // Wait for more data
  }

  // --- Frame Found ---
  uint16_t frame_len = end_pos + 1;

  // We must copy the frame to a temporary buffer because
  // rx_buffer can be modified by the USB interrupt
  uint8_t temp_frame[MAX_FRAME_SIZE];
  
  if (frame_len > MAX_FRAME_SIZE) {
      // Frame is too long, discard it
      memmove(rx_buffer, rx_buffer + frame_len, rx_index - frame_len);
      rx_index -= frame_len;
      if(rx_index > 0) data_ready = 1;
      return;
  }
  
  // --- THIS IS THE CRITICAL FIX ---
  // Copy the valid frame to a temporary buffer
  memcpy(temp_frame, rx_buffer, frame_len);

  // Clean up rx_buffer *before* processing
  // This frees the rx_buffer for the USB interrupt
  memmove(rx_buffer, rx_buffer + frame_len, rx_index - frame_len);
  rx_index -= frame_len;
  // --- END OF FIX ---

  // Check if there is more data in the buffer to process next loop
  if(rx_index > 0) data_ready = 1;


  // --- Validate and Process the temp_frame (not rx_buffer) ---
  if (!protocol_validate_frame(temp_frame, frame_len)) {
    return; // Invalid frame, ignore it
  }

  uint8_t cmd = temp_frame[0];
  uint8_t device_target = temp_frame[2];

  // --- ALL HANDLERS MUST USE temp_frame ---
  switch(cmd) {
    case CMD_QUERY_STATUS: //0x10
     if (device_target == DEVICE_PUMP) {
      pump_handle_command(temp_frame, frame_len); // CORRECTED
     } else  if( device_target == DEVICE_AGITATOR) {
      agitator_handle_command(temp_frame, frame_len); // CORRECTED
     } else if(device_target == DEVICE_ALL) {
      system_handle_unified_status(temp_frame, frame_len); // CORRECTED
     }
     break;
    
    case CMD_QUERY_SET_PARAM:
     if(device_target == DEVICE_PUMP) {
        pump_handle_command(temp_frame, frame_len); // CORRECTED
     }
     // Add agitator/system case if needed
     break;

     case CMD_START_CONTROL:  // 0x12
    if(device_target == DEVICE_PUMP) {
        pump_handle_command(temp_frame, frame_len); // CORRECTED
    } else if(device_target == DEVICE_AGITATOR) {
        agitator_handle_command(temp_frame, frame_len); // CORRECTED
    }
    break;

    case CMD_START_MULTI_PUMP: // 0x13
      if(device_target == DEVICE_PUMP) {
        pump_handle_command(temp_frame, frame_len); // This one was already correct
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

  // Note: Buffer cleanup is now at the top
}

/*
check if data ready*/
uint8_t comm_data_ready(void) { return data_ready; }