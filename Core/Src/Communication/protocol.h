#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ========================
  INTRUCTION CODE
  =========================
*/



/*=========================
  FRAME MARKERS
  ======================= */

#define FRAME_END 0xFF // ket thuc hex code

/*========================
BUFFER SIZE
=========================
*/

#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 64
#define MAX_FRAME_SIZE 32

/*=========================
  COMMAND CODES - PUMP (0x10 - 1x1F
  ==========================*/


// /**======================
//     COMMAND CODES - AGITATOR (0x20 - 0x2F)
//   ======================
// */


// /**============================
//   COMMAND CODES - SYSTEM(0x30 - 0x3F)
//   ============================
//    */


#define CMD_QUERY_STATUS 0x10 //Query any device status
#define CMD_QUERY_SET_PARAM 0x11 // Query/Set parameters or reset
#define CMD_START_CONTROL 0x12 // Start/Control operation (Single Pump)
#define CMD_START_MULTI_PUMP 0x13 // --- NEW --- Start multiple pumps
#define CMD_EMERGENCY_STOP 0x32 // Emergency stop
#define CMD_TASK_COMPLETED 0x40 // --- NEW --- Auto-notification for task finished


/*===============================
  DEVICE TARGET CODES
  ==============================
   */

#define DEVICE_PUMP 0x01 // Tartget: Pump 
#define DEVICE_AGITATOR 0x02 // Agitator system
#define DEVICE_ALL 0x03 // Target: All devices(uinified status)
#define DEVICE_SYSTEM 0x04 // Target: System-level operations

/* =============================
  OPERATION TYPE CODES
  =======================*/
#define OP_QUERY 0x55 // read operation
#define OP_SET 0xAA // Write/Set operation
#define OP_RESET 0xCC // Reset operation
/**
============================
  RESPONSE CODE
  =========================
   */

#define RESP_FAILED 0x00 //chay failed
#define RESP_SUCCESS 0x01 // Chay thanh cong

/*
=========================
    SYSTEM STATE CODES
=======================*/

#define SYS_STATE_STANDBY 0x00 // san sang cho lennh
#define SYS_STATE_RUNNING 0x01 // dang chay
#define SYS_STATE_FAULT 0x02 // dieu kien loi

/*==============================
  ERRRIR CODES
  ============================== */
#define ERR_NONE            0x00    /**< No error */
#define ERR_TIMEOUT         0x01    /**< Motor timeout */
#define ERR_OVERFLOW        0x02    /**< Overflow protection */
#define ERR_INVALID_PARAM   0x03    /**< Input khong hop le */
#define ERR_SYSTEM_BUSY     0x04    /**< System busy */

/* ================
MOTOR DEFINITIONS
==================*/
#define MOTOR_ID_1 0x01
#define MOTOR_ID_2 0x02
#define MOTOR_ID_3 0x03

// #define MIN_VOLUME_ML 10
// #define MAX_VOLUME_ML 500


// #define MIN_AGIT_DURATION 100 // it nhat 0.1s
// #define MAX_AGIT_DURATION 600000 // nhieu nhat 10p

/*=========================
  FUNCTION PROTOTYPES
  =====================*/
  uint8_t protocol_calc_checksum(uint8_t *data, uint16_t len);
  uint8_t protocol_validate_frame(uint8_t *frame, uint16_t len);
  uint16_t protocol_build_frame(uint8_t cmd, uint8_t device, uint8_t *data, uint8_t data_len, uint8_t *out_frame);

  #ifdef __cplusplus
}
#endif 

#endif
