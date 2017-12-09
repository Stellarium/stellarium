/*
    Copyright (C) 2017 by Jasem Mutlaq <mutlaqja@ikarustech.com>

    Encoder Manager

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
#include "encoderinterface.h"

namespace INDI
{

class EncoderManager
{
  public:
    EncoderManager();
    ~EncoderManager();
    std::vector<EncoderInterface *> getEncoderList();
    EncoderInterface *getEncoder();
    EncoderInterface *getDefaultEncoder();
    void setEncoder(EncoderInterface *encoder);

  protected:
    std::vector<EncoderInterface *> encoder_list;
    EncoderInterface *current_encoder;
    EncoderInterface *default_encoder;
};
}
