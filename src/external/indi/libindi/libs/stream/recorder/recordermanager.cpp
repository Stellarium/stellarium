/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    Recorder Manager

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

#include <config.h>

#include "recordermanager.h"
#include "serrecorder.h"

#ifdef HAVE_THEORA
#include "theorarecorder.h"
#endif

namespace INDI
{

RecorderManager::RecorderManager()
{
    recorder_list.push_back(new SER_Recorder());
    #ifdef HAVE_THEORA
    recorder_list.push_back(new TheoraRecorder());
    #endif
    default_recorder = recorder_list.at(0);
}

RecorderManager::~RecorderManager()
{
    std::vector<RecorderInterface *>::iterator it;
    for (it = recorder_list.begin(); it != recorder_list.end(); it++)
    {
        delete (*it);
    }
    recorder_list.clear();
}

std::vector<RecorderInterface *> RecorderManager::getRecorderList()
{
    return recorder_list;
}

RecorderInterface *RecorderManager::getRecorder()
{
    return current_recorder;
}

RecorderInterface *RecorderManager::getDefaultRecorder()
{
    return default_recorder;
}

void RecorderManager::setRecorder(RecorderInterface *recorder)
{
    current_recorder = recorder;
}

}
