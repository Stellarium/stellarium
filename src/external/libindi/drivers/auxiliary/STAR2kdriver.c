/*******************************************************************************
  created 2014 G. Schmidt

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#include "STAR2kdriver.h"

#ifndef _WIN32
#include <termios.h>
#endif

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

/* STAR2000 RS232 box control functions */

int ConnectSTAR2k(char *port);
void DisconnectSTAR2k(void);

void StartPulse(int direction);
void StopPulse(int direction);

/* Serial communication utilities (taken from celestronprotocol.c) */

typedef fd_set telfds;

static int writen(int fd, char *ptr, int nbytes);

/* some static variables */

static int STAR2kPortFD;
static int STAR2kConnectFlag = 0;

static char STAR2kOpStat = 0;

/************************* STAR2000 control functions *************************/

int ConnectSTAR2k(char *port)
{
#ifdef _WIN32
    return (-1);
#else
    struct termios tty;

    char initCmd[] = { 0x0D, 0x00 };

    fprintf(stderr, "Connecting to port: %s\n", port);

    if (STAR2kConnectFlag != 0)
        return (0);

    /* Make the connection */

    STAR2kPortFD = open(port, O_RDWR);
    if (STAR2kPortFD == -1)
        return (-1);

    tcgetattr(STAR2kPortFD, &tty);
    cfsetospeed(&tty, (speed_t)B9600);
    cfsetispeed(&tty, (speed_t)B9600);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag = IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 5;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag &= ~(PARENB | PARODD);
    tcsetattr(STAR2kPortFD, TCSANOW, &tty);

    /* Flush the input (read) buffer */

    tcflush(STAR2kPortFD, TCIOFLUSH);

    /* initialize connection */

    usleep(500000);
    writen(STAR2kPortFD, initCmd, 2);

    STAR2kOpStat = 0;

    return (0);
#endif
}

/* Start a slew in chosen direction at slewRate */
/* Use auxilliary NexStar command set through the hand control computer */

void StartPulse(int direction)
{
    if (direction == NORTH)
    {
        STAR2kOpStat |= S2K_NORTH_1;
    }
    else if (direction == EAST)
    {
        STAR2kOpStat |= S2K_EAST_1;
    }
    else if (direction == SOUTH)
    {
        STAR2kOpStat |= S2K_SOUTH_1;
    }
    else if (direction == WEST)
    {
        STAR2kOpStat |= S2K_WEST_1;
    }

    writen(STAR2kPortFD, &STAR2kOpStat, 1);
}

void StopPulse(int direction)
{
    if (direction == NORTH)
    {
        STAR2kOpStat &= S2K_NORTH_0;
    }
    else if (direction == EAST)
    {
        STAR2kOpStat &= S2K_EAST_0;
    }
    else if (direction == SOUTH)
    {
        STAR2kOpStat &= S2K_SOUTH_0;
    }
    else if (direction == WEST)
    {
        STAR2kOpStat &= S2K_WEST_0;
    }
    else if (direction == ALL)
    {
        STAR2kOpStat = 0;
    }

    writen(STAR2kPortFD, &STAR2kOpStat, 1);
}

void DisconnectSTAR2k()
{
    StopPulse(ALL);

    if (STAR2kConnectFlag == 1)
        close(STAR2kPortFD);

    STAR2kConnectFlag = 0;
}

/******************************* Serial port utilities ************************/

static int writen(int fd, char *ptr, int nbytes)
{
    int nleft, nwritten;
    nleft = nbytes;
    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0)
            break;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (nbytes - nleft);
}

/******************************************************************************/
