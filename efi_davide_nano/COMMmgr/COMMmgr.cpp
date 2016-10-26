#ifndef COMMmgr_cpp
#define COMMmgr_cpp

#include "COMMmgr.h"
#include "../compile_options.h"

uint8_t serial_inbyte[COMM_SERIAL_RECV_BYTES_NUM]; // buffer for data received by Serial (SW seriale)
uint8_t serial_byte_cnt = 0; // contatore di numero bytes ricevuti in seriale
uint8_t serial_out_bytes[COMM_SERIAL_STR_LEN_MAX]; // stringa data fuori in output per il logger, via serial
String ECU_output_string;
uint8_t data_send_req = 0; //data sending request coming from logger


// Initializes communication ports
void COMM_begin(){
	
#if (FUELINO_HW_VERSION >= 2) // Fuelino V2 has SWseriale pins (RX = 2, TX = 4)
	SWseriale.begin(); // Initializes Software seriale (Bluetooth module)
#endif

#if ENABLE_BUILT_IN_HW_SERIAL
	Serial.begin(57600); // Initializes PC serial (USB port)
#endif

}


// CALCULATED THE UBX CHECKSUM OF A CHAR ARRAY
uint16_t COMM_calculate_checksum(uint8_t* array, uint8_t array_start, uint8_t array_length) {
  uint8_t CK_A = 0;
  uint8_t CK_B = 0;
  uint8_t i;
  for (i = 0; i < array_length; i++) {
    CK_A = CK_A + array[array_start + i];
    CK_B = CK_B + CK_A;
  }
  return ((CK_A << 8) | (CK_B));
}


// Adds checksum at the end of the array, and sends it via Serial, on SWseriale
void COMM_Send_Char_Array(uint8_t* array_data, uint8_t array_size){
	
#if COMM_ENABLE_CHECKSUM
	uint16_t CK_SUM = COMM_calculate_checksum(array_data, 0, array_size);
	array_data[array_size] = CK_SUM >> 8; // CK_A
	array_data[array_size+1] = CK_SUM & 0xFF; // CK_B
	array_size += 2; // add checksum bytes
#endif
	
#if (FUELINO_HW_VERSION >= 2) && BLUETOOTH_PRESENT
	SWseriale.write(array_data, array_size); // Send complete string, including 2 bytes checksum
#endif

#if ENABLE_BUILT_IN_HW_SERIAL
	Serial.write(array_data, array_size); // Send complete string, including 2 bytes checksum
#endif
	
}


// Sends a string using SWseriale
void COMM_Send_String(String input_string, uint8_t string_size) {
  input_string.toCharArray((char*)serial_out_bytes, COMM_SERIAL_STR_LEN_MAX);
  COMM_Send_Char_Array(serial_out_bytes, string_size);
}


// Checks data coming from Serial connections
void COMM_receive_check(){
	
#if (FUELINO_HW_VERSION >= 2) // Fuelino V2 has SWseriale pins (RX = 2, TX = 4)
	while (SWseriale.available()) { // carattere ricevuto
		uint8_t temp_char_read = SWseriale.read();
		if ((temp_char_read == '\n') || (temp_char_read == '\r')){ // End of command
			if ((serial_byte_cnt != 0) && ((serial_inbyte[0] == 'e') || (serial_inbyte[0] == 'c') || (serial_inbyte[0] == 'd'))) COMM_evaluate_parameter_read_writing_request(serial_inbyte, serial_byte_cnt);
			serial_byte_cnt = 0; // restart from zero
		}
		else{ // Store character into buffer
			serial_inbyte[serial_byte_cnt] = temp_char_read;
			serial_byte_cnt++;
			if (serial_byte_cnt == COMM_SERIAL_RECV_BYTES_NUM) serial_byte_cnt = 0; // buffer is full
		}
		delay(2); // Give time to the next char to arrive
	}
#endif

#if ENABLE_BUILT_IN_HW_SERIAL	
	while (Serial.available()) { // carattere ricevuto
		uint8_t temp_char_read = Serial.read();
		if ((temp_char_read == '\n') || (temp_char_read == '\r')){ // End of command
			if ((serial_byte_cnt != 0) && ((serial_inbyte[0] == 'e') || (serial_inbyte[0] == 'c') || (serial_inbyte[0] == 'd'))) COMM_evaluate_parameter_read_writing_request(serial_inbyte, serial_byte_cnt);
			serial_byte_cnt = 0; // restart from zero
		}
		else{ // Store character into buffer
			serial_inbyte[serial_byte_cnt] = temp_char_read;
			serial_byte_cnt++;
			if (serial_byte_cnt == COMM_SERIAL_RECV_BYTES_NUM) serial_byte_cnt = 0; // buffer is full
		}
		delay(1); // Give time to the next char to arrive
	}
#endif
	
}


// Converts a character number (0-9) into an unsigned int number
uint8_t COMM_convert_char_array_to_num(uint8_t* input_array, uint8_t array_start, uint8_t array_len){
	uint16_t output_value_sum = 0;
	for (uint16_t i=0;i<array_len;i++){
		uint16_t cypher_tmp = input_array[array_start+i] - '0'; // transforms cypher into number
		if (cypher_tmp > 9) return 0x00; // not valid number
		for (uint8_t j=0; j<(array_len-i-1); j++){
			cypher_tmp*=10; // 100
		}
		output_value_sum += cypher_tmp;
	}
	if (output_value_sum > 255) return 0x00; // not valid number
	return (uint8_t)(output_value_sum & 0xFF); // converts the result in one byte
}

// Sends NACK
void COMM_send_nack(String nack_code){
	nack_code += '\n';
	COMM_Send_String(nack_code, 9); // // NACK reply
}


// Evaluates the command received from Serial Port
uint8_t COMM_evaluate_parameter_read_writing_request(uint8_t* data_array, uint8_t data_size){

	String singleMessageString = ""; // Reply message initialization
	
	// EEPROM command
	if ((data_array[0] == 'e') && (data_size == 8)){ // EEPROM command (e w xxx yyy)
		uint8_t read_write = COMM_convert_char_array_to_num(data_array, 1, 1);
		uint8_t sum_address = COMM_convert_char_array_to_num(data_array, 2, 3);
		uint8_t sum_value = COMM_convert_char_array_to_num(data_array, 5, 3);
		if ((read_write==0) && (sum_value == 0)){ // Read data
			sum_value = EEPROM.read(sum_address);
		}
		else if (read_write==1){ // Write data
			EEPROM.write(sum_address, sum_value);
		}
		else{ // Not proper format, send NACK response
			COMM_send_nack("e9999999"); // // NACK reply
			return 0; //  NG
		}
		singleMessageString += "e"; // (Header)
		singleMessageString += read_write; // Read (0) or Write (1)
		if (sum_address < 10) singleMessageString += "0"; // 3 cyphers
		if (sum_address < 100) singleMessageString += "0"; // 3 cyphers
		singleMessageString += sum_address; // 3 cyphers
		if (sum_value < 10) singleMessageString += "0"; // 3 cyphers
		if (sum_value < 100) singleMessageString += "0"; // 3 cyphers
		singleMessageString += sum_value; // 3 cyphers
		singleMessageString += '\n'; // line return
		COMM_Send_String(singleMessageString, 9); //send string
		return 1; // OK
	}
	
	// CALIBRATION command
	else if ((data_array[0] == 'c') && (data_size == 8)){
		uint8_t read_write = COMM_convert_char_array_to_num(data_array, 1, 1);
		uint8_t map_num = COMM_convert_char_array_to_num(data_array, 2, 1);
		uint8_t sum_index = COMM_convert_char_array_to_num(data_array, 3, 2);
		uint8_t sum_value = COMM_convert_char_array_to_num(data_array, 5, 3);
		singleMessageString += "c"; // (Header)
		singleMessageString += read_write; // Read (0) or Write (1)
		singleMessageString += map_num; // Map number
		if ((read_write==0) && (sum_value == 0)){ // Read from RAM
			sum_value = 0xFF; // tentative value (error case)
			if (sum_index < 10) singleMessageString += "0"; // Index
			singleMessageString += sum_index; // Index
			if ((map_num == 0) && (sum_index < INJ_INCR_RPM_MAPS_SIZE)){
				sum_value = incrementi_rpm[sum_index];
			}
			else if ((map_num == 1) && (sum_index < INJ_INCR_THR_MAPS_SIZE)){
				sum_value = incrementi_thr[sum_index];
			}
			else if ((map_num == 2) && (sum_index < INJ_INCR_TIM_MAPS_SIZE)){
				sum_value = incrementi_tim[sum_index];
			}else{
				COMM_send_nack("c0999999"); // // NACK reply
				return 0; // NG
			}
			if (sum_value < 10) singleMessageString += "0"; // 3 cyphers
			if (sum_value < 100) singleMessageString += "0"; // 3 cyphers
			singleMessageString += sum_value; // 3 cyphers
			singleMessageString += '\n'; // line return
			COMM_Send_String(singleMessageString, 9); //send string
			return 1; // OK
		}
		else if ((read_write==1)){ // Write to RAM
			if ((map_num == 0) && (sum_index < INJ_INCR_RPM_MAPS_SIZE)){
				incrementi_rpm[sum_index]=sum_value; // write the value in RAM
				sum_value=incrementi_rpm[sum_index]; // check back
			}
			else if ((map_num == 1) && (sum_index < INJ_INCR_THR_MAPS_SIZE)){
				incrementi_thr[sum_index]=sum_value; // write the value in RAM
				sum_value=incrementi_thr[sum_index]; // check back
			}
			else if ((map_num == 2) && (sum_index < INJ_INCR_TIM_MAPS_SIZE)){
				incrementi_tim[sum_index]=sum_value; // write the value in RAM
				sum_value=incrementi_tim[sum_index]; // check back
			}else{
				COMM_send_nack("c1999999"); // // NACK reply
				return 0; // NG
			}
			if (sum_index < 10) singleMessageString += "0"; // Index
			singleMessageString += sum_index; // Index
			if (sum_value < 10) singleMessageString += "0"; // 3 cyphers
			if (sum_value < 100) singleMessageString += "0"; // 3 cyphers
			singleMessageString += sum_value; // 3 cyphers
			singleMessageString += '\n'; // line return
			COMM_Send_String(singleMessageString, 9); //send string
			return 1; // OK
		}
		else if ((read_write==2) && (map_num<=INJ_MAPS_TOTAL_NUM) && (sum_index == 0)){ // Write to EEPROM
			singleMessageString += "0000"; // 00 00
			if (sum_value == 0){
				EEPROM_write_standard_values(map_num); // c n 2 00 000
				singleMessageString += "0"; // 0
			}else if (sum_value == 1){
				EEPROM_write_RAM_map_to_EEPROM(map_num); // c n 2 00 001
				singleMessageString += "1"; // 1
			}else{
				COMM_send_nack("c2999999"); // // NACK reply
				return 0; // NG
			}
			singleMessageString += '\n'; // line return
			COMM_Send_String(singleMessageString, 9); //send string
			return 1; // OK
		}
		else{
			COMM_send_nack("c9999999"); // // NACK reply
			return 0; // Error
		}
	}
	
	// DATA command
	else if ((data_array[0] == 'd') && (data_size >= 4) && (data_size <= 8)){ // DATA command (d x yy ...)
		if (data_array[1] == '0'){ // d 0 ... (ASCII request)
			uint8_t request_num = COMM_convert_char_array_to_num(data_array, 2, 2);
			uint16_t val_to_send;
			bool req_good = false;
			singleMessageString += "d0"; // (Header) d 0
			if (request_num < 10) singleMessageString += "0"; // 2 cyphers
			singleMessageString += request_num; // 2 cyphers
			if (request_num == 0){ // d 0 0 0 ... // 2rpm
				val_to_send = delta_time_tick_buffer;
				req_good = true;
			}
			else if (request_num == 1){ // d 0 0 1 ... // inj time
				val_to_send = delta_inj_tick_buffer;
				req_good = true;
			}
			else if (request_num == 2){ // d 0 0 2 ... // extension time
				val_to_send = extension_time_ticks_buffer;
				req_good = true;
			}
			else if (request_num == 3){ // d 0 0 3 ... // throttle
				val_to_send = throttle_buffer;
				req_good = true;
			}
			else if (request_num == 4){ // d 0 0 4 ... // lambda
				val_to_send = lambda_buffer;
				req_good = true;
			}
			else if (request_num == 5){ // d 0 0 5 ... // combustion counter
				val_to_send = injection_counter_buffer;
				req_good = true;
			}
			if (req_good == true){
				if (val_to_send < 10) singleMessageString += "0"; // 5 cyphers
				if (val_to_send < 100) singleMessageString += "0"; // 5 cyphers
				if (val_to_send < 1000) singleMessageString += "0"; // 5 cyphers
				if (val_to_send < 10000) singleMessageString += "0"; // 5 cyphers
				singleMessageString += val_to_send; // 5 cyphers
				singleMessageString += '\n'; // line return
				COMM_Send_String(singleMessageString, 10); //send string
				return 1; // OK
			}
		}
		else if ((data_array[1] == '1') && (data_array[2] == '0') && (data_array[3] == '0')){ // d 1 0 0 ... (binary request)
			
			#if (FUELINO_HW_VERSION >= 2) && BLUETOOTH_PRESENT
			SWseriale.write(SDmgr.SD_writing_buffer, SDmgr.SD_writing_buffer_size); // send packet
			#endif
			
			#if ENABLE_BUILT_IN_HW_SERIAL
			Serial.write(SDmgr.SD_writing_buffer, SDmgr.SD_writing_buffer_size); // send packet
			#endif
			
			return 1; // OK
		}
		COMM_send_nack("d9999999"); // // NACK reply
		return 0;
	}
	
	COMM_send_nack("99999999"); // // NACK reply
	return 0; // Error
}

#endif