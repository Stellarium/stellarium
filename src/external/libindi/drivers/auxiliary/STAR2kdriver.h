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

#pragma once

/********************************** some defines ******************************/

#define S2K_WEST_1  0x01
#define S2K_SOUTH_1 0x02
#define S2K_NORTH_1 0x04
#define S2K_EAST_1  0x08

#define S2K_WEST_0  0x0E
#define S2K_SOUTH_0 0x0D
#define S2K_NORTH_0 0x0B
#define S2K_EAST_0  0x07

#define NORTH 0
#define WEST  1
#define EAST  2
#define SOUTH 3

#define ALL -1

#ifdef __cplusplus
extern "C" {
#endif

int ConnectSTAR2k(char *port);
void DisconnectSTAR2k();

void StartPulse(int direction);
void StopPulse(int direction);

#ifdef __cplusplus
}
#endif
