/**
 * Copyright © 2021 Timothy Reaves
 *
 * For the license, see the root LICENSE file.
 */

#pragma once

#include <QString>

/* ***************************************************************************************************************** */
//                                          Keys for use in QSettings
const QLatin1String Telescopes("telescope");
const QLatin1String TelescopeName("name");
const QLatin1String TelescopeFocalLength("focalLength");
const QLatin1String TelescopeDiameter("diameter");
const QLatin1String TelescopeFlipHorizontal("hFlip");
const QLatin1String TelescopeFlipVertical("vFlip");
const QLatin1String TelescopeEquatorial("equatorial");

/* ***************************************************************************************************************** */
//                                Numeric Constants (prevents Magic Number warnings)
const double        CCDDefaultChipHeigth          = 15.7;
const double        CCDDefaultChipWidth           = 23.4;
const int           CCDDefaultResolutionX         = 6224;
const int           CCDDefaultResolutionY         = 4168;
const double        OcularDefaultFOV              = 68.0;
const double        OcularDefaultFocalLength      = 32.0;
const double        TelescopeDefaultDiameter      = 80.0;
const double        TelescopeDefaultFocalLength   = 500.0;

const double        DegreesPerRadian              = 57.2958;

const double        MIN_OCULARS_INI_VERSION       = 3.1f;
const int           DEFAULT_CCD_CROP_OVERLAY_SIZE = 250;
