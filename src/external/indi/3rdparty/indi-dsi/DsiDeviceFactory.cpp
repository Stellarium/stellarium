/*
 * Copyright © 2008, Roland Roberts
 * Copyright © 2015, Ben Gilsrud
 */

#include "DsiDeviceFactory.h"

#include "DsiPro.h"
#include "DsiColor.h"
#include "DsiProII.h"
#include "DsiColorII.h"
#include "DsiProIII.h"
#include "DsiColorIII.h"
#include "DsiException.h"

#include <iomanip>
#include <iostream>

DSI::Device *DSI::DeviceFactory::getInstance(const char *devname)
{
    DSI::Device *tmp;
    // Yes, we open the device in order to find out what CCD it is, then we do
    // it all over again creating a specific subtype.
    try
    {
        tmp = new DSI::Device(devname);
    }
    catch (dsi_exception e)
    {
        std::cerr << e.what();
        return NULL;
    }

    std::cerr << "Found Camera " << tmp->getCameraName() << std::endl << "Found CCD " << tmp->getCcdChipName() << std::endl;

    std::string ccdChipName = tmp->getCcdChipName();
    delete tmp;

    if (ccdChipName == "ICX254AL")
        return new DSI::DsiPro(devname);

    if (ccdChipName == "ICX404AK")
        return new DSI::DsiColor(devname);

    if (ccdChipName == "ICX429ALL")
        return new DSI::DsiProII(devname);

    if (ccdChipName == "ICX429AKL")
        return new DSI::DsiColorII(devname);

    if (ccdChipName == "ICX285AL")
        return new DSI::DsiProIII(devname);

    if (ccdChipName == "ICX285AQ")
        return new DSI::DsiColorIII(devname);

    return 0;
}
