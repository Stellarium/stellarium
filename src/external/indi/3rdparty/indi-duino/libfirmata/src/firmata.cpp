/*
 * Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)

   Base on the following works:
	* Firmata GUI example. http://www.pjrc.com/teensy/firmata_test/
	  Copyright 2010, Paul Stoffregen (paul at pjrc.com)

	* firmataplus: http://sourceforge.net/projects/firmataplus/
	  Copyright (c) 2008 - Scott Reid, dataczar.com

   Firmata C++ library. 
*/

#include <firmata.h>
#include <string.h>

int debug = 0;

Firmata::Firmata()
{
    init("/dev/ttyACM0");
}

Firmata::Firmata(const char *_serialPort)
{
    init(_serialPort);
}

Firmata::~Firmata()
{
    delete arduino;
}

int Firmata::writeDigitalPin(unsigned char pin, unsigned char mode)
{
    int rv = 0;
    int bit;
    int port;
    if (pin < 8 && pin > 1)
    {
        port = 0;
        bit  = pin;
    }
    else if (pin > 7 && pin < 14)
    {
        port = 1;
        bit  = pin - 8;
    }
    else if (pin > 15 && pin < 22)
    {
        port = 2;
        bit  = pin - 16;
    }
    else
    {
        return (-2);
    }
    // set the bit
    if (mode == ARDUINO_HIGH)
        digitalPortValue[port] |= (1 << bit);

    // clear the bit
    else if (mode == ARDUINO_LOW)
        digitalPortValue[port] &= ~(1 << bit);

    else
    {
        perror("Firmata::writeDigitalPin():invalid mode:");
        return (-1);
    }
    rv |= arduino->sendUchar(FIRMATA_DIGITAL_MESSAGE + port);
    rv |= sendValueAsTwo7bitBytes(digitalPortValue[port]); //ARDUINO_HIGH OR ARDUINO_LOW
    return (rv);
}

// in Firmata (and MIDI) data bytes are 7-bits. The 8th bit serves as a flag to mark a byte as either command or data.
// therefore you need two data bytes to send 8-bits (a char).
int Firmata::sendValueAsTwo7bitBytes(int value)
{
    int rv;
    rv |= arduino->sendUchar(value & 127);      // LSB
    rv |= arduino->sendUchar(value >> 7 & 127); // MSB
    return rv;
}

int Firmata::setSamplingInterval(int16_t value)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_SAMPLING_INTERVAL);
    rv |= arduino->sendUchar((unsigned char)(value % 128));
    rv |= arduino->sendUchar((unsigned char)(value >> 7));
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    return (rv);
}

int Firmata::setPinMode(unsigned char pin, unsigned char mode)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_SET_PIN_MODE);
    rv |= arduino->sendUchar(pin);
    rv |= arduino->sendUchar(mode);
    usleep(1000);
    askPinState(pin);
    return (rv);
}

int Firmata::setPwmPin(unsigned char pin, int16_t value)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_ANALOG_MESSAGE + pin);
    rv |= arduino->sendUchar((unsigned char)(value % 128));
    rv |= arduino->sendUchar((unsigned char)(value >> 7));
    return (rv);
}
int Firmata::mapAnalogChannels()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_ANALOG_MAPPING_QUERY); // read firmata name & version
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    return (rv);
}

int Firmata::askFirmwareVersion()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_REPORT_FIRMWARE); // read firmata name & version
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    return (rv);
}

int Firmata::askCapabilities()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_CAPABILITY_QUERY);
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    return (rv);
}

int Firmata::askPinState(int pin)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_PIN_STATE_QUERY);
    rv |= arduino->sendUchar(pin);
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    rv |= usleep(1000);
    rv |= OnIdle();
    return (rv);
}

int Firmata::reportDigitalPorts(int enable)
{
    int rv = 0;
    for (int i = 0; i < 20; i++)
    {
        rv |= arduino->sendUchar(FIRMATA_REPORT_DIGITAL | i); // report analog
        rv |= arduino->sendUchar(enable);
    }
    return (rv);
}

int Firmata::reportAnalogPorts(int enable)
{
    int rv = 0;
    for (int i = 0; i < 20; i++)
    {
        rv |= arduino->sendUchar(FIRMATA_REPORT_ANALOG | i); // report analog
        rv |= arduino->sendUchar(enable);
    }
    return (rv);
}

int Firmata::systemReset()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_SYSTEM_RESET);
    return (rv);
}

int Firmata::closePort()
{
    if (arduino->closePort() < 0)
    {
        perror("Firmata::closePort():arduino->closePort():");
        portOpen = 0;
        return (-1);
    }
    return (0);
}

int Firmata::flushPort()
{
    if (arduino->flushPort() < 0)
    {
        perror("Firmata::flushPort():arduino->flushPort():");
        return (-1);
    }
    return (0);
}

int Firmata::sendStringData(char *data)
{
    //TODO Testting
    int rv = 0;
    int i;
    char c;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_STRING_DATA);
    for (int i = 0; i < strlen(data); i++)
    {
        rv |= sendValueAsTwo7bitBytes(data[i]);
    }
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    return (rv);
}

int Firmata::init(const char *_serialPort)
{
    arduino  = new Arduino();
    portOpen = 0;
    if (arduino->openPort(_serialPort, FIRMATA_DEFAULT_BAUD) != 0)
    {
        if (debug)
            fprintf(stderr, "sf->openPort(%s) failed: exiting\n", _serialPort);
        return 1;
    }

    askFirmwareVersion();
    usleep(1000);
    while (true)
    {
        OnIdle();
        if (strlen(firmata_name) > 0)
        {
            if (debug)
                printf("FIRMATA ARDUINO BOARD:%s\n", firmata_name);
            fflush(stdout);
            portOpen = 1;
            break;
        }
    }
    askCapabilities();
    usleep(1000);
    OnIdle();
    mapAnalogChannels();
    usleep(1000);
    OnIdle();
    for (int pin = 0; pin < 20; pin++)
    {
        askPinState(pin);
    }
    return 0;
}

void Firmata::Parse(const uint8_t *buf, int len)
{
    const uint8_t *p, *end;

    p   = buf;
    end = p + len;
    for (p = buf; p < end; p++)
    {
        uint8_t msn = *p & 0xF0;
        if (msn == 0xE0 || msn == 0x90 || *p == 0xF9)
        {
            parse_command_len = 3;
            parse_count       = 0;
        }
        else if (msn == 0xC0 || msn == 0xD0)
        {
            parse_command_len = 2;
            parse_count       = 0;
        }
        else if (*p == FIRMATA_START_SYSEX)
        {
            parse_count       = 0;
            parse_command_len = sizeof(parse_buf);
        }
        else if (*p == FIRMATA_END_SYSEX)
        {
            parse_command_len = parse_count + 1;
        }
        else if (*p & 0x80)
        {
            parse_command_len = 1;
            parse_count       = 0;
        }
        if (parse_count < (int)sizeof(parse_buf))
        {
            parse_buf[parse_count++] = *p;
        }
        if (parse_count == parse_command_len)
        {
            DoMessage();
            parse_count = parse_command_len = 0;
        }
    }
}

void Firmata::DoMessage(void)
{
    uint8_t cmd = (parse_buf[0] & 0xF0);

    //if (debug) printf("message, %d bytes, %02X\n", parse_count, parse_buf[0]);

    if (cmd == FIRMATA_ANALOG_MESSAGE && parse_count == 3)
    {
        int analog_ch  = (parse_buf[0] & 0x0F);
        int analog_val = parse_buf[1] | (parse_buf[2] << 7);
        for (int pin = 0; pin < 128; pin++)
        {
            if (pin_info[pin].analog_channel == analog_ch)
            {
                pin_info[pin].value = analog_val;
                if (debug)
                    printf("pin %d is A%d = %d\n", pin, analog_ch, analog_val);
                return;
            }
        }
        return;
    }
    if (cmd == FIRMATA_DIGITAL_MESSAGE && parse_count == 3)
    {
        int port_num = (parse_buf[0] & 0x0F);
        int port_val = parse_buf[1] | (parse_buf[2] << 7);
        int pin      = port_num * 8;
        if (debug)
            printf("port_num = %d, port_val = %d\n", port_num, port_val);
        for (int mask = 1; mask & 0xFF; mask <<= 1, pin++)
        {
            if (pin_info[pin].mode == FIRMATA_MODE_INPUT)
            {
                uint32_t val = (port_val & mask) ? 1 : 0;
                if (pin_info[pin].value != val)
                {
                    if (debug)
                        printf("pin %d is %d\n", pin, val);
                    pin_info[pin].value = val;
                }
            }
        }
        return;
    }

    if (parse_buf[0] == FIRMATA_START_SYSEX && parse_buf[parse_count - 1] == FIRMATA_END_SYSEX)
    {
        // Sysex message
        if (parse_buf[1] == FIRMATA_REPORT_FIRMWARE)
        {
            char name[140];
            int len = 0;
            for (int i = 4; i < parse_count - 2; i += 2)
            {
                name[len++] = (parse_buf[i] & 0x7F) | ((parse_buf[i + 1] & 0x7F) << 7);
            }
            name[len++] = '-';
            name[len++] = parse_buf[2] + '0';
            name[len++] = '.';
            name[len++] = parse_buf[3] + '0';
            name[len++] = 0;
            strcpy(firmata_name, name);
            //if (debug) printf("FIRMWARE:%s\n",firmata_name);
        }
        else if (parse_buf[1] == FIRMATA_CAPABILITY_RESPONSE)
        {
            int pin, i, n;
            for (pin = 0; pin < 128; pin++)
            {
                pin_info[pin].supported_modes = 0;
            }
            for (i = 2, n = 0, pin = 0; i < parse_count; i++)
            {
                if (parse_buf[i] == 127)
                {
                    pin++;
                    n = 0;
                    continue;
                }
                if (n == 0)
                {
                    // first byte is supported mode
                    pin_info[pin].supported_modes |= (1 << parse_buf[i]);
                    if (debug)
                        printf("PIN:%u modes:%04x\n", pin, (short)pin_info[pin].supported_modes);
                }
                n = n ^ 1;
            }
        }
        else if (parse_buf[1] == FIRMATA_ANALOG_MAPPING_RESPONSE)
        {
            int pin = 0;
            for (int i = 2; i < parse_count - 1; i++)
            {
                pin_info[pin].analog_channel = parse_buf[i];
                pin++;
            }
            return;
        }
        else if (parse_buf[1] == FIRMATA_PIN_STATE_RESPONSE && parse_count >= 6)
        {
            int pin             = parse_buf[2];
            pin_info[pin].mode  = parse_buf[3];
            pin_info[pin].value = parse_buf[4];
            if (parse_count > 6)
                pin_info[pin].value |= (parse_buf[5] << 7);
            if (parse_count > 7)
                pin_info[pin].value |= (parse_buf[6] << 14);
            if (debug)
                printf("PIN:%u. Mode:%u. Value:%lu\n", pin, pin_info[pin].mode, pin_info[pin].value);
        }
        else if (parse_buf[1] == FIRMATA_STRING_DATA)
        {
            if ((parse_count - 3) >= MAX_STRING_DATA_LEN)
            {
                if (debug)
                    printf("FIRMATA_STRING_DATA TOO LARGE.%u Parsing up to max %u\n", (parse_count - 3),
                           MAX_STRING_DATA_LEN);
                parse_count = FIRMATA_STRING_DATA + 3;
            }
            char name[MAX_STRING_DATA_LEN];
            int len = 0;
            for (int i = 2; i < parse_count - 2; i += 2)
            {
                name[len++] = (parse_buf[i] & 0x7F) | ((parse_buf[i + 1] & 0x7F) << 7);
            }
            name[len++] = 0;
            strcpy(string_buffer, name);
        }
        else if (parse_buf[1] == FIRMATA_EXTENDED_ANALOG)
        {
            //TODO Testting
            if ((parse_count - 3) > 8)
                printf("Extended analog max precision uint64_bit");
            int pin = (parse_buf[2] & 0x7F); //UP to 128 analogs
            if (pin_info[pin].mode == FIRMATA_MODE_INPUT)
            {
                int analog_val = (parse_buf[3] & 0x7F);
                for (int i = 4; i < parse_count - 1; i++)
                {
                    analog_val = (analog_val << 7) | (parse_buf[i] & 0x7F);
                }
                pin_info[pin].value = analog_val;
                if (debug)
                    printf("Extended analog: pin %d = %d\n", pin, analog_val);
            }
        }
        else if (parse_buf[1] == FIRMATA_I2C_REPLY)
        {
            //TODO Testting
            if ((parse_count - 3) > 8)
                printf("I2C_REPLY max precision uint64_bit (8 bytes)");
            int slaveAddress = (parse_buf[2] & 0x7F);
            slaveAddress     = (slaveAddress << 7) || (parse_buf[3] & 0x7F);
            long i2c_val     = (parse_buf[4] & 0x7F);
            for (int i = 4; i < parse_count - 1; i++)
            {
                i2c_val = (i2c_val << 7) | (parse_buf[i] & 0x7F);
            }
            if (debug)
                printf("I2C_REPLY value: SlaveAddres %u = %ld\n", slaveAddress, i2c_val);
        }
        return;
    }
}

int Firmata::OnIdle()
{
    uint8_t buf[1024];
    int r = 1;

    //if (debug) printf("Idle event\n");
    if (r > 0)
    {
        r = arduino->readPort(buf, sizeof(buf));
        if (r < 0)
        {
            // error
            return r;
        }
        if (r > 0)
        {
            for (int i = 0; i < r; i++)
            {
                if (debug)
                    printf("%02X ", buf[i]);
            }
            if (debug)
                printf("\n");

            Parse(buf, r);
            return 0;
        }
    }
    else if (r < 0)
    {
        return r;
    }
}
