/*
 * Copyright © 2009, Roland Roberts
 *
 */

#pragma once

#include "DsiDevice.h"

namespace DSI
{
class DeviceFactory
{
  public:
    static Device *getInstance(const char *devpath = 0);
};
};
