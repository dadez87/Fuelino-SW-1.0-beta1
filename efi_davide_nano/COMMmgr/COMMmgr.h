#ifndef COMMmgr_h
#define COMMmgr_h

#include <Arduino.h>
#include "SWSeriale/SWseriale.h" // SW serial using INT1 and Timer2"
#include "../INJmgr/INJmgr.h"
#include "../EEPROMmgr/EEPROMmgr.h"
#include "../SDmgr/SDmgr.h"

#define COMM_SERIAL_RECV_BYTES_NUM 32
#define COMM_SERIAL_STR_LEN_MAX 16
#define COMM_ENABLE_CHECKSUM 0

// Global variables to be exported
extern uint8_t data_send_req; //data sending request coming from logger

// Functions to be exported
extern void COMM_begin();
extern uint16_t COMM_calculate_checksum(uint8_t* array, uint8_t array_start, uint8_t array_length);
extern void COMM_Send_Char_Array(uint8_t* array_data, uint8_t array_size);
extern void COMM_Send_String(String input_string, uint8_t string_size);
extern void COMM_receive_check();
extern uint8_t COMM_evaluate_parameter_read_writing_request(uint8_t *data_array, uint8_t data_size);

#endif
