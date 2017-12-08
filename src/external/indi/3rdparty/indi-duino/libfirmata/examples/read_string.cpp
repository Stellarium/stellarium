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
    if (argc < 2)
    {
        fprintf(stderr, "Usage: read_string <serial port path> \n");
        exit(1);
    }
    char *serial = argv[1];
    Firmata *sf  = new Firmata(serial);
    while (true)
    {
        sf->OnIdle();
        usleep(10000);
        printf("ARDUINO SAY: %s\n", sf->string_buffer);
    }

    delete sf;
    return 0;
}
