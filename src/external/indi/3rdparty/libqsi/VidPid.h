/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) QSI 2012
 * USB Vendor ID and Product ID as a pair
 */

#ifndef _VIDPID_H_
#define _VIDPID_H_

class VidPid
{
public:
	VidPid();
	VidPid(int vid, int pid);
	~VidPid();
	VidPid(const VidPid& vidpid);

	int VID;
	int PID;

protected:

private:

};

#endif // _VIDPID_H_
