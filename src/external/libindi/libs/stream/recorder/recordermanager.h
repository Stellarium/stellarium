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

#include <vector>
#include "recorderinterface.h"

namespace INDI
{

class RecorderManager
{
  public:
    RecorderManager();
    ~RecorderManager();
    std::vector<RecorderInterface *> getRecorderList();
    RecorderInterface *getRecorder();
    RecorderInterface *getDefaultRecorder();
    void setRecorder(RecorderInterface *recorder);

  protected:
    std::vector<RecorderInterface *> recorder_list;
    RecorderInterface *current_recorder;
    RecorderInterface *default_recorder;
};
}
