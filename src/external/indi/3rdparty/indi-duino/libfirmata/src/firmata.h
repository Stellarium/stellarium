/*
 * Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)

   Base on the following works:
	* Firmata GUI example. http://www.pjrc.com/teensy/firmata_test/
	  Copyright 2010, Paul Stoffregen (paul at pjrc.com)

	* firmataplus: http://sourceforge.net/projects/firmataplus/
	  Copyright (c) 2008 - Scott Reid, dataczar.com

   Firmata C++ library. 
*/

#include <vector>
#include <stdint.h>
#include <arduino.h>

#define FIRMATA_MAX_DATA_BYTES 32 // max number of data bytes in non-Sysex messages
//#define FIRMATA_DEFAULT_BAUD          115200
#define FIRMATA_DEFAULT_BAUD          57600
#define FIRMATA_FIRMWARE_VERSION_SIZE 2 // number of bytes in firmware version

// message command bytes (128-255/0x80-0xFF)
#define FIRMATA_DIGITAL_MESSAGE 0x90 // send data for a digital pin
#define FIRMATA_ANALOG_MESSAGE  0xE0 // send data for an analog pin (or PWM)
#define FIRMATA_REPORT_ANALOG   0xC0 // enable analog input by pin #
#define FIRMATA_REPORT_DIGITAL  0xD0 // enable digital input by port pair
//
#define FIRMATA_SET_PIN_MODE 0xF4 // set a pin to INPUT/OUTPUT/PWM/etc
//
#define FIRMATA_REPORT_VERSION 0xF9 // report protocol version
#define FIRMATA_SYSTEM_RESET   0xFF // reset from MIDI
//
#define FIRMATA_START_SYSEX 0xF0 // start a MIDI Sysex message
#define FIRMATA_END_SYSEX   0xF7 // end a MIDI Sysex message

// extended command set using sysex (0-127/0x00-0x7F)
/* 0x00-0x0F reserved for custom commands */

#define FIRMATA_RESERVED_COMMAND        0x00 // 2nd SysEx data byte is a chip-specific command (AVR, PIC, TI, etc).
#define FIRMATA_ANALOG_MAPPING_QUERY    0x69 // ask for mapping of analog to pin numbers
#define FIRMATA_ANALOG_MAPPING_RESPONSE 0x6A // reply with mapping info
#define FIRMATA_CAPABILITY_QUERY        0x6B // ask for supported modes and resolution of all pins
#define FIRMATA_CAPABILITY_RESPONSE     0x6C // reply with supported modes and resolution
#define FIRMATA_PIN_STATE_QUERY         0x6D // ask for a pin's current mode and value
#define FIRMATA_PIN_STATE_RESPONSE      0x6E // reply with a pin's current mode and value
#define FIRMATA_EXTENDED_ANALOG         0x6F // analog write (PWM, Servo, etc) to any pin
#define FIRMATA_SERVO_CONFIG            0x70 // set max angle, minPulse, maxPulse, freq
#define FIRMATA_STRING_DATA             0x71 // a string message with 14-bits per char
#define FIRMATA_SHIFT_DATA              0x75 // shiftOut config/data message (34 bits)
#define FIRMATA_I2C_REQUEST             0x76 // I2C request messages from a host to an I/O board
#define FIRMATA_I2C_REPLY               0x77 // I2C reply messages from an I/O board to a host
#define FIRMATA_I2C_CONFIG              0x78 // Configure special I2C settings such as power pins and delay times
#define FIRMATA_REPORT_FIRMWARE         0x79 // report name and version of the firmware
#define FIRMATA_SAMPLING_INTERVAL       0x7A // sampling interval
#define FIRMATA_SYSEX_NON_REALTIME      0x7E // MIDI Reserved for non-realtime messages
#define FIRMATA_SYSEX_REALTIME          0x7F // MIDI Reserved for realtime messages

// pin modes
#define FIRMATA_MODE_INPUT  0x00
#define FIRMATA_MODE_OUTPUT 0x01
#define FIRMATA_MODE_ANALOG 0x02
#define FIRMATA_MODE_PWM    0x03
#define FIRMATA_MODE_SERVO  0x04
#define FIRMATA_MODE_SHIFT  0x05
#define FIRMATA_MODE_I2C    0x06

#define FIRMATA_I2C_WRITE                   B00000000
#define FIRMATA_I2C_READ                    B00001000
#define FIRMATA_I2C_READ_CONTINUOUSLY       B00010000
#define FIRMATA_I2C_STOP_READING            B00011000
#define FIRMATA_I2C_READ_WRITE_MODE_MASK    B00011000
#define FIRMATA_I2C_10BIT_ADDRESS_MODE_MASK B00100000

#define MAX_STRING_DATA_LEN 164

using namespace std;

typedef struct
{
    uint8_t mode;
    uint8_t analog_channel;
    uint64_t supported_modes;
    uint64_t value;
} pin_t;

class Firmata
{
  public:
    Firmata();
    Firmata(const char *_serialPort);
    ~Firmata();

    int writeDigitalPin(unsigned char pin, unsigned char mode); // mode can be ARDUINO_HIGH or ARDUINO_LOW
    int setPinMode(unsigned char pin, unsigned char mode);
    int setPwmPin(unsigned char pin, int16_t value);
    int mapAnalogChannels();
    int askFirmwareVersion();
    int askCapabilities();
    int askPinState(int pin);
    int reportDigitalPorts(int enable);
    int reportAnalogPorts(int enable);
    int setSamplingInterval(int16_t value);
    int systemReset();
    int closePort();
    int flushPort();
    //int getSysExData();
    int sendStringData(char *data);
    pin_t pin_info[128];
    void print_state();
    char firmata_name[140];
    char string_buffer[MAX_STRING_DATA_LEN];
    int OnIdle();
    bool portOpen;

  private:
    int parse_count;
    int parse_command_len;
    uint8_t parse_buf[4096];
    void Parse(const uint8_t *buf, int len);
    void DoMessage(void);

  protected:
    Arduino *arduino;

    int waitForData;
    int executeMultiByteCommand;
    int multiByteChannel;
    unsigned char serialInBuf[FIRMATA_MAX_DATA_BYTES];
    unsigned char serialOutBuf[FIRMATA_MAX_DATA_BYTES];

    vector<unsigned char> sysExBuf;
    char firmwareVersion[FIRMATA_FIRMWARE_VERSION_SIZE];
    int digitalPortValue[ARDUINO_DIG_PORTS]; /// bitpacked digital pin state
    int init(const char *_serialPort);
    int sendValueAsTwo7bitBytes(int value);
};
