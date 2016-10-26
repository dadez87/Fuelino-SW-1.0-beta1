#ifndef CompileOptions_h
#define CompileOptions_h

#define FUELINO_HW_VERSION 1 // HW version of Fuelino. Fuelino V1 does not have SWseriale, and also, for injector management, input and output pins are different
#define ENABLE_BUILT_IN_HW_SERIAL 1 // Enables the communication HW Serial port (USB connection with PC) on pins 0 and 1
#define SD_MODULE_PRESENT 1 // SD Card module present
#define DISPLAY_PRESENT 0 // Display module on I2C
#define IMU_PRESENT 0 // IMU module on I2C
#define GPS_PRESENT 0 // GPS module on SW Serial
#define BLUETOOTH_PRESENT 1 // Enables packets forwarding (sending and receiving) through SW Serial, in case FUELINO_HW_VERSION>=2

#endif
