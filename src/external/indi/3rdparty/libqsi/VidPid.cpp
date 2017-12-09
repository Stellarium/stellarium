/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) QSI 2012
 * 
 */

#include "VidPid.h"

VidPid::VidPid()
{
	VID = 0x0403;
	PID = 0xeb48;
}

VidPid::VidPid(int vid, int pid)
{
	VID = vid;
	PID = pid;
}

VidPid::~VidPid()
{

}

VidPid::VidPid(const VidPid & vidpid)
{
	this->VID    = vidpid.VID;
	this->PID    = vidpid.PID;
}
