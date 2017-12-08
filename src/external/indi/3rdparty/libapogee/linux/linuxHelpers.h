/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \namespace linuxHelpers
* \brief helper class for Linux OSs
* 
*/ 

#ifndef LINUXHELPERS_H_
#define LINUXHELPERS_H_

#include <vector>
#include <stdint.h>

namespace linuxHelpers
{
	std::vector< std::vector<uint16_t> > GetDevicesLinux();
}

#endif /* LINUXHELPERS_H_ */
