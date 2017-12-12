/*
    Copyright (C) 2005 by Jasem Mutlaq <mutlaqja@ikarustech.com>
    Copyright (C) 2014 by geehalel <geehalel@gmail.com>

    V4L2 Decode

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

#include "v4l2_decode.h"
#include "v4l2_builtin_decoder.h"

V4L2_Decoder::V4L2_Decoder()
{
}

V4L2_Decoder::~V4L2_Decoder()
{
}

const char *V4L2_Decoder::getName()
{
    return name;
}

V4L2_Decode::V4L2_Decode()
{
    decoder_list.push_back(new V4L2_Builtin_Decoder());
    default_decoder = decoder_list.at(0);
}

V4L2_Decode::~V4L2_Decode()
{
    std::vector<V4L2_Decoder *>::iterator it;
    for (it = decoder_list.begin(); it != decoder_list.end(); it++)
    {
        delete (*it);
    }
    decoder_list.clear();
}

std::vector<V4L2_Decoder *> V4L2_Decode::getDecoderList()
{
    return decoder_list;
}

V4L2_Decoder *V4L2_Decode::getDecoder()
{
    return current_decoder;
}
V4L2_Decoder *V4L2_Decode::getDefaultDecoder()
{
    return default_decoder;
};
void V4L2_Decode::setDecoder(V4L2_Decoder *decoder)
{
    current_decoder = decoder;
};
