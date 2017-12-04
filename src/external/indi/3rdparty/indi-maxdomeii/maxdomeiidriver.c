/*
    Max Dome II Driver
    Copyright (C) 2009 Ferran Casarramona (ferran.casarramona@gmail.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "maxdomeiidriver.h"

#include <indicom.h>

#define MAXDOME_TIMEOUT 5  /* FD timeout in seconds */
#define MAX_BUFFER      15 // Message length can be up to 12 bytes.

// Start byte
#define START 0x01

// Message Destination
#define TO_MAXDOME  0x00
#define TO_COMPUTER 0x80

// Commands available
#define ABORT_CMD   0x03 // Abort azimuth movement
#define HOME_CMD    0x04 // Move until 'home' position is detected
#define GOTO_CMD    0x05 // Go to azimuth position
#define SHUTTER_CMD 0x06 // Send a command to Shutter
#define STATUS_CMD  0x07 // Retrieve status
#define TICKS_CMD   0x09 // Set the number of tick per revolution of the dome
#define ACK_CMD     0x0A // ACK (?)
#define SETPARK_CMD 0x0B // Set park coordinates and if need to park before to operating shutter

// Shutter commands
#define OPEN_SHUTTER            0x01
#define OPEN_UPPER_ONLY_SHUTTER 0x02
#define CLOSE_SHUTTER           0x03
#define EXIT_SHUTTER            0x04 // Command send to shutter on program exit
#define ABORT_SHUTTER           0x07

// Error messages
const char *ErrorMessages[] = { "Ok",                              // no error
                                "No response from MAX DOME",       // -1
                                "Invalid declared message length", // -2
                                "Message too short",               // -3
                                "Checksum error",                  // -4
                                "Could not send command",          // -5
                                "Response do not match command",   // -6
                                "" };

/*
	Opens serial port with proper configuration
	
	@param device String to device (/dev/ttyS0)
	@return -1 if can't connect. File descriptor, otherwise.
*/
int Connect_MaxDomeII(const char *device)
{
    int fd;

    if (tty_connect(device, 19200, 8, 0, 1, &fd) != TTY_OK)
    {
        return -1;
    }

    //fd = OpenSerialPort(device);

    return fd;
}

/*
	Inform Max Dome from a disconection and closes serial port
	
	@param fd File descriptor
	@return 0 Ok, 
*/
int Disconnect_MaxDomeII(int fd)
{
    Exit_Shutter_MaxDomeII(fd); // Really don't know why this is needed, but ASCOM driver does it.
    tty_disconnect(fd);

    return 0;
}

/*
	Calculates or checks the checksum
	It ignores first byte
	For security we limit the checksum calculation to MAX_BUFFER length
	
	@param cMessage Pointer to unsigned char array with the message
	@param nLen Length of the message  
	@return Checksum byte
*/
signed char checksum_MaxDomeII(char *cMessage, int nLen)
{
    int nIdx;
    char nChecksum = 0;

    for (nIdx = 1; nIdx < nLen && nIdx < MAX_BUFFER; nIdx++)
    {
        nChecksum -= cMessage[nIdx];
    }

    return nChecksum;
}

/*
	Reads a response from MAX DOME II
	It verifies message sintax and checksum
	
	@param fd File descriptor
	@param cMessage Pointer to a buffer to receive the message
	@return 0 if message is Ok. -1 if no response or no start caracter found. -2 invalid declared message length. -3 message too short. -4 checksum error
*/
int ReadResponse_MaxDomeII(int fd, char *cMessage)
{
    int nBytesRead;
    int nLen       = MAX_BUFFER;
    int nErrorType = TTY_OK;
    char nChecksum;

    *cMessage    = 0x00;
    cMessage[13] = 0x00;
    // Look for a 0x01 starting character, until time out occurs or MAX_BUFFER characters was read
    while (*cMessage != 0x01 && nErrorType == TTY_OK)
    {
        nErrorType = tty_read(fd, cMessage, 1, MAXDOME_TIMEOUT, &nBytesRead);
        //fprintf(stderr,"\nIn 1: %ld %02x\n", nBytesRead, (int)cMessage[0]);
    }

    if (nErrorType != TTY_OK || *cMessage != 0x01)
        return -1;

    // Read message length
    nErrorType = tty_read(fd, cMessage + 1, 1, MAXDOME_TIMEOUT, &nBytesRead);

    //fprintf(stderr,"\nInLen: %d\n",(int) cMessage[1]);
    if (nErrorType != TTY_OK || cMessage[1] < 0x02 || cMessage[1] > 0x0E)
        return -2;

    nLen = cMessage[1];
    // Read the rest of the message
    nErrorType = tty_read(fd, cMessage + 2, nLen, MAXDOME_TIMEOUT, &nBytesRead);

    //fprintf(stderr,"\nIn: %s\n", cMessage);
    if (nErrorType != TTY_OK || nBytesRead != nLen)
        return -3;

    nChecksum = checksum_MaxDomeII(cMessage, nLen + 2);
    if (nChecksum != 0x00)
        return -4;

    return 0;
}

/*
	Abort azimuth movement
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Abort_Azimuth_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x02; // Will follow 2 bytes more
    cMessage[2] = ABORT_CMD;
    cMessage[3] = checksum_MaxDomeII(cMessage, 3);
    nErrorType  = tty_write(fd, cMessage, 4, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(ABORT_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Move until 'home' position is detected
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Home_Azimuth_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x02; // Will follow 2 bytes more
    cMessage[2] = HOME_CMD;
    cMessage[3] = checksum_MaxDomeII(cMessage, 3);
    nErrorType  = tty_write(fd, cMessage, 4, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(HOME_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Go to a new azimuth position
	
	@param fd File descriptor
	@param nDir Direcction of the movement. 0 E to W. 1 W to E
	@param nTicks Ticks from home position in E to W direction.
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Goto_Azimuth_MaxDomeII(int fd, int nDir, int nTicks)
{
    char cMessage[MAX_BUFFER];
    int nErrorType = 0;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x05; // Will follow 5 bytes more
    cMessage[2] = GOTO_CMD;
    cMessage[3] = (char)nDir;
    // Note: we not use nTicks >> 8 in order to remain compatible with both little-endian and big-endian procesors
    cMessage[4] = (char)(nTicks / 256);
    cMessage[5] = (char)(nTicks % 256);
    cMessage[6] = checksum_MaxDomeII(cMessage, 6);
    nErrorType  = tty_write(fd, cMessage, 7, &nBytesWrite);

    //fprintf(stderr,"\nOut: %ld %02x %02x %02x %02x %02x %02x %02x\n", nBytesWrite, (int)cMessage[0], (int)cMessage[1], (int)cMessage[2], (int)cMessage[3], (int)cMessage[4], (int)cMessage[5], (int)cMessage[6]);
    //if (nErrorType != TTY_OK)
    if (nBytesWrite != 7)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(GOTO_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Ask Max Dome status
	
	@param fd File descriptor
	@param nShutterStatus Returns shutter status
	@param nAzimuthStatus Returns azimuth status
	@param nAzimuthPosition Returns azimuth current position (in ticks from home position)
	@param nHomePosition Returns last position where home was detected
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Status_MaxDomeII(int fd, enum SH_Status *nShutterStatus, enum AZ_Status *nAzimuthStatus, unsigned *nAzimuthPosition,
                     unsigned *nHomePosition)
{
    unsigned char cMessage[MAX_BUFFER];
    int nErrorType = 0;
    int nBytesWrite = 0;
    int nReturn = 0;

    cMessage[0] = 0x01;
    cMessage[1] = 0x02; // Will follow 2 bytes more
    cMessage[2] = STATUS_CMD;
    cMessage[3] = checksum_MaxDomeII((char*)cMessage, 3);
    nErrorType  = tty_write(fd, (char*)cMessage, 4, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, (char*)cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (unsigned char)(STATUS_CMD | TO_COMPUTER))
    {
        *nShutterStatus   = (int)cMessage[3];
        *nAzimuthStatus   = (int)cMessage[4];
        *nAzimuthPosition = (unsigned)(((unsigned)cMessage[5]) * 256 + ((unsigned)cMessage[6]));
        *nHomePosition    = ((unsigned)cMessage[7]) * 256 + ((unsigned)cMessage[8]);

        return 0;
    }

    return -6; // Response don't match command
}

/*
	Ack comunication
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Ack_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType = TTY_OK;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x02; // Will follow 2 bytes more
    cMessage[2] = ACK_CMD;
    cMessage[3] = checksum_MaxDomeII(cMessage, 3);
    nErrorType  = tty_write(fd, cMessage, 4, &nBytesWrite);
    //nBytesWrite = write(fd, cMessage, 4);
    //if (nBytesWrite != 4)
    //	nErrorType = TTY_WRITE_ERROR;
    //fprintf(stderr,"\nOut: %ld %02x %02x %02x %02x\n", nBytesWrite, (int)cMessage[0], (int)cMessage[1], (int)cMessage[2], (int)cMessage[3]);
    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    //fprintf(stderr,"\nIn: %02x %02x %02x %02x %02x\n", cMessage[0], cMessage[1], cMessage[2], cMessage[3], cMessage[4]);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(ACK_CMD | TO_COMPUTER))
        return 0;

    //fprintf(stderr,"\nIn: %02x %02x", (unsigned char)(ACK_CMD | TO_COMPUTER), cMessage[2]);
    return -6; // Response don't match command
}

/*
	Set park coordinates and if need to park before to operating shutter
	
	@param fd File descriptor
	@param nParkOnShutter 0 operate shutter at any azimuth. 1 go to park position before operating shutter
	@param nTicks Ticks from home position in E to W direction.
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int SetPark_MaxDomeII(int fd, int nParkOnShutter, int nTicks)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x05; // Will follow 5 bytes more
    cMessage[2] = SETPARK_CMD;
    cMessage[3] = (char)nParkOnShutter;
    // Note: we not use nTicks >> 8 in order to remain compatible with both little-endian and big-endian procesors
    cMessage[4] = (char)(nTicks / 256);
    cMessage[5] = (char)(nTicks % 256);
    cMessage[6] = checksum_MaxDomeII(cMessage, 6);
    nErrorType  = tty_write(fd, cMessage, 7, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(SETPARK_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
 Set ticks per turn of the dome
 
 @param fd File descriptor
 @param nTicks Ticks from home position in E to W direction.
 @return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
 */
int SetTicksPerCount_MaxDomeII(int fd, int nTicks)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x04; // Will follow 4 bytes more
    cMessage[2] = TICKS_CMD;
    // Note: we not use nTicks >> 8 in order to remain compatible with both little-endian and big-endian procesors
    cMessage[3] = (char)(nTicks / 256);
    cMessage[4] = (char)(nTicks % 256);
    cMessage[5] = checksum_MaxDomeII(cMessage, 5);
    nErrorType  = tty_write(fd, cMessage, 6, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(TICKS_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

///////////////////////////////////////////////////////////////////////////
//
//  Shutter commands
//
///////////////////////////////////////////////////////////////////////////

/*
	Opens the shutter fully
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Open_Shutter_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x03; // Will follow 3 bytes more
    cMessage[2] = SHUTTER_CMD;
    cMessage[3] = OPEN_SHUTTER;
    cMessage[4] = checksum_MaxDomeII(cMessage, 4);
    nErrorType  = tty_write(fd, cMessage, 5, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(SHUTTER_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Opens the upper shutter only
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Open_Upper_Shutter_Only_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x03; // Will follow 3 bytes more
    cMessage[2] = SHUTTER_CMD;
    cMessage[3] = OPEN_UPPER_ONLY_SHUTTER;
    cMessage[4] = checksum_MaxDomeII(cMessage, 4);
    nErrorType  = tty_write(fd, cMessage, 5, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(SHUTTER_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Close the shutter
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Close_Shutter_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x03; // Will follow 3 bytes more
    cMessage[2] = SHUTTER_CMD;
    cMessage[3] = CLOSE_SHUTTER;
    cMessage[4] = checksum_MaxDomeII(cMessage, 4);
    nErrorType  = tty_write(fd, cMessage, 5, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(SHUTTER_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Abort shutter movement
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Abort_Shutter_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x03; // Will follow 3 bytes more
    cMessage[2] = SHUTTER_CMD;
    cMessage[3] = ABORT_SHUTTER;
    cMessage[4] = checksum_MaxDomeII(cMessage, 4);
    nErrorType  = tty_write(fd, cMessage, 5, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(SHUTTER_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}

/*
	Exit shutter (?) Normally send at program exit
	
	@param fd File descriptor
	@return 0 command received by MAX DOME. -5 Couldn't send command. -6 Response don't match command. See ReadResponse() return
*/
int Exit_Shutter_MaxDomeII(int fd)
{
    char cMessage[MAX_BUFFER];
    int nErrorType;
    int nBytesWrite;
    int nReturn;

    cMessage[0] = 0x01;
    cMessage[1] = 0x03; // Will follow 3 bytes more
    cMessage[2] = SHUTTER_CMD;
    cMessage[3] = EXIT_SHUTTER;
    cMessage[4] = checksum_MaxDomeII(cMessage, 4);
    nErrorType  = tty_write(fd, cMessage, 5, &nBytesWrite);

    if (nErrorType != TTY_OK)
        return -5;

    nReturn = ReadResponse_MaxDomeII(fd, cMessage);
    if (nReturn != 0)
        return nReturn;

    if (cMessage[2] == (char)(SHUTTER_CMD | TO_COMPUTER))
        return 0;

    return -6; // Response don't match command
}
