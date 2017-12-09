/*****************************************************************************************
NAME
 QSI API External Trigger Demo Application

DESCRIPTION
 Simple QSI API External Trigger example

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2012

REVISION HISTORY
 DRC 08.04.12 Original Version
 *****************************************************************************************/

#include "qsiapi.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <cmath>
#include <stdlib.h>

		int main(int argc, char** argv)
        {
            QSICamera cam;

            try
            {
				cam.put_UseStructuredExceptions(true);
                cam.put_Connected(true);
            }
            catch (std::runtime_error &err)
            {
				std::cout << "Cannot connect to camera." << std::endl;
                exit(1);
            }
            ////////////////////////////////////////////////////////////////////////////
            // Short Wait Trigger Mode
            ////////////////////////////////////////////////////////////////////////////

            // Set Short Wait External Trigger (4 seconds max), Pos to Neg polarity
            cam.EnableTriggerMode(QSICamera::ShortWait, QSICamera::HighToLow);
			bool imageReady = false;
            try
            {
				std::cout << "Start short wait." << std::endl;
                cam.StartExposure(0.3, true);  
 				// Short wait, so this will return with an image, or will timeout
				cam.get_ImageReady(&imageReady);
                while (!imageReady)
                {
                    sleep(1);
					cam.get_ImageReady(&imageReady);
                }
                long x;
				cam.get_NumX(&x);
                long y;
				cam.get_NumY(&y);
                long len = x * y;

                unsigned short * pixels = new unsigned short[len];
                cam.get_ImageArray(pixels);
				//Process image
				//Then clean up
				delete [] pixels;
				std::cout << "Short wait image complete." << std::endl;
            }
            catch (std::runtime_error &err)
            {
                // Timeout comes here
				std::cout << "Short wait timeout." << std::endl;
            }

            // turn off external trigger mode
            cam.CancelTriggerMode();

            /////////////////////////////////////////////////////////////////////////
            // Long Wait Trigger Mode
            ////////////////////////////////////////////////////////////////////////

            cam.EnableTriggerMode(QSICamera::LongWait, QSICamera::LowToHigh);
            try
            {
				std::cout << "Start long wait with cancel." << std::endl;
                // Start a long wait exposure
                cam.StartExposure(0.3, true);
                // Sleep for 5 seconds as a demo
                sleep(5);
                //
                // Demo cancelling of the pending trigger
                // This would not normally be done...
                cam.TerminatePendingTrigger();
				std::cout << "Long wait pending conceled." << std::endl;
            }
            catch (std::runtime_error &err)
            {
				std::cout << "Long wait cancel exception." << err.what() << std::endl;
				exit(1);
            }

            // Start a new exposure with long wait trigger.  
			// Note that the trigger mode remains in effect until canceled.
            try
            {
				std::cout << "Start long wait for image." << std::endl;
                cam.StartExposure(0.3, true);
				cam.get_ImageReady(&imageReady);
                while (!imageReady)
                {
                    sleep(1);
					cam.get_ImageReady(&imageReady);
                }
                long x;
				cam.get_NumX(&x);
                long y;
				cam.get_NumY(&y);
                long len = x * y;

                unsigned short * pixels = new unsigned short[len];
                cam.get_ImageArray(pixels);
				// Process image
				// Then clean up
				delete [] pixels;
				std::cout << "Long wait image complete." << std::endl;
            }
            catch (std::runtime_error &err)
            {
				std::cout << "Long wait exception." << err.what() << std::endl;
				exit(1);
            }

            ////////////////////////////////////////////////////////////////////
            // Cancel Trigger Mode
            ////////////////////////////////////////////////////////////////////
            cam.CancelTriggerMode();
            cam.put_Connected(false);
			std::cout << "Trigger test completed." << std::endl;
        }
