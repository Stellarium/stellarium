//
//    FILE: dht.cpp
// VERSION: 0.1.05
// PURPOSE: DHT Temperature & Humidity Sensor library for Arduino
//
// DATASHEET:
//
// HISTORY:
// 0.1.05 fixed negative temperature bug (thanks to Roseman)
// 0.1.04 improved readability of code using DHTLIB_OK in code
// 0.1.03 added error values for temp and humidity when read failed
// 0.1.02 added error codes
// 0.1.01 added support for Arduino 1.0, fixed typos (31/12/2011)
// 0.1.0 by Rob Tillaart (01/04/2011)
// inspired by DHT11 library
//

#include "dht.h"

#define TIMEOUT 10000UL

/////////////////////////////////////////////////////
//
// PUBLIC
//

// return values:
// DHTLIB_OK
// DHTLIB_ERROR_CHECKSUM
// DHTLIB_ERROR_TIMEOUT
int dht::read11(uint8_t pin)
{
    // READ VALUES
    int rv = read(pin);
    if (rv != DHTLIB_OK)
    {
        humidity    = DHTLIB_INVALID_VALUE; // or is NaN prefered?
        temperature = DHTLIB_INVALID_VALUE;
        return rv;
    }

    // CONVERT AND STORE
    humidity    = bits[0]; // bit[1] == 0;
    temperature = bits[2]; // bits[3] == 0;

    // TEST CHECKSUM
    uint8_t sum = bits[0] + bits[2]; // bits[1] && bits[3] both 0
    if (bits[4] != sum)
        return DHTLIB_ERROR_CHECKSUM;

    return DHTLIB_OK;
}

// return values:
// DHTLIB_OK
// DHTLIB_ERROR_CHECKSUM
// DHTLIB_ERROR_TIMEOUT
int dht::read22(uint8_t pin)
{
    // READ VALUES
    int rv = read(pin);
    if (rv != DHTLIB_OK)
    {
        humidity    = DHTLIB_INVALID_VALUE; // invalid value, or is NaN prefered?
        temperature = DHTLIB_INVALID_VALUE; // invalid value
        return rv;
    }

    // CONVERT AND STORE
    humidity = word(bits[0], bits[1]) * 0.1;

    if (bits[2] & 0x80) // negative temperature
    {
        temperature = word(bits[2] & 0x7F, bits[3]) * 0.1;
        temperature *= -1.0;
    }
    else
    {
        temperature = word(bits[2], bits[3]) * 0.1;
    }

    // TEST CHECKSUM
    uint8_t sum = bits[0] + bits[1] + bits[2] + bits[3];
    if (bits[4] != sum)
        return DHTLIB_ERROR_CHECKSUM;

    return DHTLIB_OK;
}

/////////////////////////////////////////////////////
//
// PRIVATE
//

// return values:
// DHTLIB_OK
// DHTLIB_ERROR_TIMEOUT
int dht::read(uint8_t pin)
{
    // INIT BUFFERVAR TO RECEIVE DATA
    uint8_t cnt = 7;
    uint8_t idx = 0;

    // EMPTY BUFFER
    for (int i = 0; i < 5; i++)
        bits[i] = 0;

    // REQUEST SAMPLE
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(20);
    digitalWrite(pin, HIGH);
    delayMicroseconds(40);
    pinMode(pin, INPUT);

    // GET ACKNOWLEDGE or TIMEOUT
    unsigned int loopCnt = TIMEOUT;
    while (digitalRead(pin) == LOW)
        if (loopCnt-- == 0)
            return DHTLIB_ERROR_TIMEOUT;

    loopCnt = TIMEOUT;
    while (digitalRead(pin) == HIGH)
        if (loopCnt-- == 0)
            return DHTLIB_ERROR_TIMEOUT;

    // READ THE OUTPUT - 40 BITS => 5 BYTES
    for (int i = 0; i < 40; i++)
    {
        loopCnt = TIMEOUT;
        while (digitalRead(pin) == LOW)
            if (loopCnt-- == 0)
                return DHTLIB_ERROR_TIMEOUT;

        unsigned long t = micros();

        loopCnt = TIMEOUT;
        while (digitalRead(pin) == HIGH)
            if (loopCnt-- == 0)
                return DHTLIB_ERROR_TIMEOUT;

        if ((micros() - t) > 40)
            bits[idx] |= (1 << cnt);
        if (cnt == 0) // next byte?
        {
            cnt = 7;
            idx++;
        }
        else
            cnt--;
    }

    return DHTLIB_OK;
}
//
// END OF FILE
//
