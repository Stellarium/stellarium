/*
 * Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)

   Base on the following works:
	* Firmata GUI example. http://www.pjrc.com/teensy/firmata_test/
	  Copyright 2010, Paul Stoffregen (paul at pjrc.com)

	* firmataplus: http://sourceforge.net/projects/firmataplus/
	  Copyright (c) 2008 - Scott Reid, dataczar.com

   Firmata C++ library. 
*/

#include <iostream>
#include <stdlib.h>
#include <firmata.h>

int main(int argc, char **argv)
{
    int rv = 0;
    if (argc < 2)
    {
        fprintf(stderr, "Usage: blink_pin <serial port path> [pin]\n");
        exit(1);
    }
    char *serial = argv[1];
    int pin      = atoi(argv[2]);
    Firmata *sf  = new Firmata(serial);
    if (!sf->portOpen)
    {
        printf("Fail to open port %s.Exiting\n", serial);
        exit(1);
    }
    if (!sf->setPinMode(pin, FIRMATA_MODE_OUTPUT))
    {
        printf("Fail to set pin mode for pin:%u as OUTPUT.Exiting\n", pin);
        exit(1);
    }
    for (int j = 0; j < 10; j++)
    {
        printf("PORT %u HIGH\n", pin);
        sf->writeDigitalPin(pin, ARDUINO_HIGH);
        sleep(1);
        printf("PORT %u LOW\n", pin);
        sf->writeDigitalPin(pin, ARDUINO_LOW);
        sleep(1);
    }
    delete sf;
    return 0;
}
