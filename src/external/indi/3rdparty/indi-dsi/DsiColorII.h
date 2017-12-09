/*
 * Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 * Copyright (c) 2015, Ben Gilsrud <bgilsrud@gmail.com>
 *
 */

#pragma once

#include "DsiDevice.h"

namespace DSI
{
class DsiColorII : public Device
{
  private:
  protected:
    void initImager(const char *devname = 0);

  public:
    DsiColorII(const char *devname = 0);
    ~DsiColorII();
};
};
