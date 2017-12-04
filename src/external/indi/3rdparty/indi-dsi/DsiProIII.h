/*
 * Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 *
 */

#pragma once

#include "DsiDevice.h"

namespace DSI
{
class DsiProIII : public Device
{
  private:
    void initImager(const char *devname = 0);

  public:
    DsiProIII(const char *devname);
    ~DsiProIII();
};
};
