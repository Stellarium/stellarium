/*******************************************************************************
  Copyright(c) 2010 Gerry Rozema. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "ccd_simulator.h"
#include "stream/streammanager.h"

#include "locale_compat.h"

#include <libnova/julian_day.h>
#include <libnova/precession.h>

#include <cmath>
#include <unistd.h>

pthread_cond_t cv         = PTHREAD_COND_INITIALIZER;
pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;

#define POLLMS  1000

// We declare an auto pointer to ccdsim.
std::unique_ptr<CCDSim> ccdsim(new CCDSim());

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    ccdsim->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    ccdsim->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ccdsim->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    ccdsim->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    ccdsim->ISSnoopDevice(root);
}

CCDSim::CCDSim() : INDI::FilterInterface(this)
{    
    raPE  = RA;
    decPE = Dec;

    streamPredicate = 0;
    terminateThread = false;

    primaryFocalLength = 900; //  focal length of the telescope in millimeters
    guiderFocalLength  = 300;

    time(&RunStart);

    SimulatorSettingsNV = new INumberVectorProperty;
    TimeFactorSV        = new ISwitchVectorProperty;

    // Filter stuff
    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 8;
}

bool CCDSim::SetupParms()
{
    int nbuf;
    SetCCDParams(SimulatorSettingsN[0].value, SimulatorSettingsN[1].value, 16, SimulatorSettingsN[2].value,
                 SimulatorSettingsN[3].value);

    if (HasCooler())
    {
        TemperatureN[0].value = 20;
        IDSetNumber(&TemperatureNP, nullptr);
    }

    //  Kwiq
    maxnoise      = SimulatorSettingsN[8].value;
    skyglow       = SimulatorSettingsN[9].value;
    maxval        = SimulatorSettingsN[4].value;
    bias          = SimulatorSettingsN[5].value;
    limitingmag   = SimulatorSettingsN[7].value;
    saturationmag = SimulatorSettingsN[6].value;
    OAGoffset = SimulatorSettingsN[10].value; //  An oag is offset this much from center of scope position (arcminutes);
    polarError = SimulatorSettingsN[11].value;
    polarDrift = SimulatorSettingsN[12].value;
    rotationCW = SimulatorSettingsN[13].value;

    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8;
    //nbuf += 512;
    PrimaryCCD.setFrameBufferSize(nbuf);

    Streamer->setPixelFormat(INDI_MONO, 16);
    Streamer->setSize(PrimaryCCD.getXRes(), PrimaryCCD.getYRes());

    return true;
}

bool CCDSim::Connect()
{
    pthread_create(&primary_thread, nullptr, &streamVideoHelper, this);
    return true;
}

bool CCDSim::Disconnect()
{
    pthread_mutex_lock(&condMutex);
    streamPredicate = 1;
    terminateThread = true;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&condMutex);

    return true;
}

const char *CCDSim::getDefaultName()
{
    return (const char *)"CCD Simulator";
}

bool CCDSim::initProperties()
{
    //  Most hardware layers wont actually have indi properties defined
    //  but the simulators are a special case
    INDI::CCD::initProperties();

    IUFillNumber(&SimulatorSettingsN[0], "SIM_XRES", "CCD X resolution", "%4.0f", 0, 8192, 0, 1280);
    IUFillNumber(&SimulatorSettingsN[1], "SIM_YRES", "CCD Y resolution", "%4.0f", 0, 8192, 0, 1024);
    IUFillNumber(&SimulatorSettingsN[2], "SIM_XSIZE", "CCD X Pixel Size", "%4.2f", 0, 60, 0, 5.2);
    IUFillNumber(&SimulatorSettingsN[3], "SIM_YSIZE", "CCD Y Pixel Size", "%4.2f", 0, 60, 0, 5.2);
    IUFillNumber(&SimulatorSettingsN[4], "SIM_MAXVAL", "CCD Maximum ADU", "%4.0f", 0, 65000, 0, 65000);
    IUFillNumber(&SimulatorSettingsN[5], "SIM_BIAS", "CCD Bias", "%4.0f", 0, 6000, 0, 10);
    IUFillNumber(&SimulatorSettingsN[6], "SIM_SATURATION", "Saturation Mag", "%4.1f", 0, 20, 0, 1.0);
    IUFillNumber(&SimulatorSettingsN[7], "SIM_LIMITINGMAG", "Limiting Mag", "%4.1f", 0, 20, 0, 17.0);
    IUFillNumber(&SimulatorSettingsN[8], "SIM_NOISE", "CCD Noise", "%4.0f", 0, 6000, 0, 10);
    IUFillNumber(&SimulatorSettingsN[9], "SIM_SKYGLOW", "Sky Glow (magnitudes)", "%4.1f", 0, 6000, 0, 19.5);
    IUFillNumber(&SimulatorSettingsN[10], "SIM_OAGOFFSET", "Oag Offset (arcminutes)", "%4.1f", 0, 6000, 0, 0);
    IUFillNumber(&SimulatorSettingsN[11], "SIM_POLAR", "PAE (arcminutes)", "%4.1f", -600, 600, 0,
                 0); /* PAE = Polar Alignment Error */
    IUFillNumber(&SimulatorSettingsN[12], "SIM_POLARDRIFT", "PAE Drift (minutes)", "%4.1f", 0, 6000, 0, 0);
    IUFillNumber(&SimulatorSettingsN[13], "SIM_ROTATION", "Rotation CW (degrees)", "%4.1f", -360, 360, 0, 0);
    IUFillNumberVector(SimulatorSettingsNV, SimulatorSettingsN, 14, getDeviceName(), "SIMULATOR_SETTINGS",
                       "Simulator Settings", "Simulator Config", IP_RW, 60, IPS_IDLE);

    IUFillSwitch(&TimeFactorS[0], "1X", "Actual Time", ISS_ON);
    IUFillSwitch(&TimeFactorS[1], "10X", "10x", ISS_OFF);
    IUFillSwitch(&TimeFactorS[2], "100X", "100x", ISS_OFF);
    IUFillSwitchVector(TimeFactorSV, TimeFactorS, 3, getDeviceName(), "ON_TIME_FACTOR", "Time Factor",
                       "Simulator Config", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillNumber(&FWHMN[0], "SIM_FWHM", "FWHM (arcseconds)", "%4.2f", 0, 60, 0, 7.5);
    IUFillNumberVector(&FWHMNP, FWHMN, 1, ActiveDeviceT[1].text, "FWHM", "FWHM", OPTIONS_TAB, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&CoolerS[0], "COOLER_ON", "ON", ISS_OFF);
    IUFillSwitch(&CoolerS[1], "COOLER_OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&CoolerSP, CoolerS, 2, getDeviceName(), "CCD_COOLER", "Cooler", MAIN_CONTROL_TAB, IP_WO,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillNumber(&EqPEN[0], "RA_PE", "RA (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumber(&EqPEN[1], "DEC_PE", "DEC (dd:mm:ss)", "%010.6m", -90, 90, 0, 0);
    IUFillNumberVector(&EqPENP, EqPEN, 2, ActiveDeviceT[0].text, "EQUATORIAL_PE", "EQ PE", "Main Control", IP_RW, 60,
                       IPS_IDLE);

    IDSnoopDevice(ActiveDeviceT[0].text, "EQUATORIAL_PE");
    IDSnoopDevice(ActiveDeviceT[1].text, "FWHM");

    uint32_t cap = 0;

    cap |= CCD_CAN_ABORT;
    cap |= CCD_CAN_BIN;
    cap |= CCD_CAN_SUBFRAME;
    cap |= CCD_HAS_COOLER;
    cap |= CCD_HAS_GUIDE_HEAD;
    cap |= CCD_HAS_SHUTTER;
    cap |= CCD_HAS_ST4_PORT;
    cap |= CCD_HAS_STREAMING;

    SetCCDCapability(cap);

    INDI::FilterInterface::initProperties(FILTER_TAB);

    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 8;

    addDebugControl();

    setDriverInterface(getDriverInterface() | FILTER_INTERFACE);

    return true;
}

void CCDSim::ISGetProperties(const char *dev)
{
    //  First we let our parent populate

    //IDLog("CCDSim IsGetProperties with %s\n",dev);
    INDI::CCD::ISGetProperties(dev);

    defineNumber(SimulatorSettingsNV);
    defineSwitch(TimeFactorSV);
}

bool CCDSim::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        if (HasCooler())
            defineSwitch(&CoolerSP);

        SetupParms();

        if (HasGuideHead())
        {
            SetGuiderParams(500, 290, 16, 9.8, 12.6);
            GuideCCD.setFrameBufferSize(GuideCCD.getXRes() * GuideCCD.getYRes() * 2);
        }

        // Define the Filter Slot and name properties
        INDI::FilterInterface::updateProperties();

        updatePeriodMS = POLLMS;
        SetTimer(updatePeriodMS);
    }
    else
    {
        if (HasCooler())
            deleteProperty(CoolerSP.name);

        INDI::FilterInterface::updateProperties();
    }

    return true;
}

int CCDSim::SetTemperature(double temperature)
{
    TemperatureRequest = temperature;
    if (fabs(temperature - TemperatureN[0].value) < 0.1)
    {
        TemperatureN[0].value = temperature;
        return 1;
    }

    CoolerS[0].s = ISS_ON;
    CoolerS[1].s = ISS_OFF;
    CoolerSP.s   = IPS_BUSY;
    IDSetSwitch(&CoolerSP, nullptr);
    return 0;
}

bool CCDSim::StartExposure(float duration)
{
    if (std::isnan(RA) && std::isnan(Dec))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Telescope coordinates missing. Make sure telescope is connected and its name is set in CCD Options.");
        return false;
    }

    //  for the simulator, we can just draw the frame now
    //  and it will get returned at the right time
    //  by the timer routines
    AbortPrimaryFrame = false;
    ExposureRequest   = duration;

    PrimaryCCD.setExposureDuration(duration);
    gettimeofday(&ExpStart, nullptr);
    //  Leave the proper time showing for the draw routines
    DrawCcdFrame(&PrimaryCCD);
    //  Now compress the actual wait time
    ExposureRequest = duration * TimeFactor;
    InExposure      = true;
    return true;
}

bool CCDSim::StartGuideExposure(float n)
{
    GuideExposureRequest = n;
    AbortGuideFrame      = false;
    GuideCCD.setExposureDuration(n);
    DrawCcdFrame(&GuideCCD);
    gettimeofday(&GuideExpStart, nullptr);
    InGuideExposure = true;
    return true;
}

bool CCDSim::AbortExposure()
{
    if (!InExposure)
        return true;

    AbortPrimaryFrame = true;

    return true;
}

bool CCDSim::AbortGuideExposure()
{
    //IDLog("Enter AbortGuideExposure\n");
    if (!InGuideExposure)
        return true; //  no need to abort if we aren't doing one
    AbortGuideFrame = true;
    return true;
}

float CCDSim::CalcTimeLeft(timeval start, float req)
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };
    gettimeofday(&now, nullptr);

    timesince =
        (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) - (double)(start.tv_sec * 1000.0 + start.tv_usec / 1000);
    timesince = timesince / 1000;
    timeleft  = req - timesince;
    return timeleft;
}

void CCDSim::TimerHit()
{
    uint32_t nextTimer = updatePeriodMS;

    if (!isConnected())
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        if (AbortPrimaryFrame)
        {
            InExposure        = false;
            AbortPrimaryFrame = false;
        }
        else
        {
            float timeleft;
            timeleft = CalcTimeLeft(ExpStart, ExposureRequest);

            //IDLog("CCD Exposure left: %g - Requset: %g\n", timeleft, ExposureRequest);
            if (timeleft < 0)
                timeleft = 0;

            PrimaryCCD.setExposureLeft(timeleft);

            if (timeleft < 1.0)
            {
                if (timeleft <= 0.001)
                {
                    InExposure = false;
                    PrimaryCCD.binFrame();
                    ExposureComplete(&PrimaryCCD);
                }
                else
                {
                    nextTimer = timeleft * 1000; //  set a shorter timer
                }
            }
        }
    }

    if (InGuideExposure)
    {
        float timeleft;
        timeleft = CalcTimeLeft(GuideExpStart, GuideExposureRequest);

        //IDLog("GUIDE Exposure left: %g - Requset: %g\n", timeleft, GuideExposureRequest);

        if (timeleft < 0)
            timeleft = 0;

        //ImageExposureN[0].value = timeleft;
        //IDSetNumber(ImageExposureNP, nullptr);
        GuideCCD.setExposureLeft(timeleft);

        if (timeleft < 1.0)
        {
            if (timeleft <= 0.001)
            {
                InGuideExposure = false;
                if (!AbortGuideFrame)
                {
                    GuideCCD.binFrame();
                    ExposureComplete(&GuideCCD);
                    if (InGuideExposure)
                    {
                        //  the call to complete triggered another exposure
                        timeleft = CalcTimeLeft(GuideExpStart, GuideExposureRequest);
                        if (timeleft < 1.0)
                        {
                            nextTimer = timeleft * 1000;
                        }
                    }
                }
                else
                {
                    //IDLog("Not sending guide frame cuz of abort\n");
                }
                AbortGuideFrame = false;
            }
            else
            {
                nextTimer = timeleft * 1000; //  set a shorter timer
            }
        }
    }

    if (TemperatureNP.s == IPS_BUSY)
    {
        if (fabs(TemperatureRequest - TemperatureN[0].value) <= 0.5)
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "Temperature reached requested value %.2f degrees C", TemperatureRequest);
            TemperatureN[0].value = TemperatureRequest;
            TemperatureNP.s       = IPS_OK;
        }
        else
        {
            if (TemperatureRequest < TemperatureN[0].value)
                TemperatureN[0].value -= 0.5;
            else
                TemperatureN[0].value += 0.5;
        }

        IDSetNumber(&TemperatureNP, nullptr);

        // Above 20, cooler is off
        if (TemperatureN[0].value >= 20)
        {
            CoolerS[0].s = ISS_OFF;
            CoolerS[0].s = ISS_ON;
            CoolerSP.s   = IPS_IDLE;
            IDSetSwitch(&CoolerSP, nullptr);
        }
    }

    SetTimer(nextTimer);
}

int CCDSim::DrawCcdFrame(INDI::CCDChip *targetChip)
{
    //  CCD frame is 16 bit data
    uint16_t val;
    float ExposureTime;
    float targetFocalLength;

    uint16_t *ptr = reinterpret_cast<uint16_t*>(targetChip->getFrameBuffer());

    if (targetChip->getXRes() == 500)
        ExposureTime = GuideExposureRequest;
    else if (Streamer->isStreaming())
        ExposureTime = (ExposureRequest < 1) ? (ExposureRequest * 100) : ExposureRequest * 2;
    else
        ExposureTime = ExposureRequest;

    if (TelescopeTypeS[TELESCOPE_PRIMARY].s == ISS_ON)
        targetFocalLength = primaryFocalLength;
    else
        targetFocalLength = guiderFocalLength;

    if (ShowStarField)
    {
        char gsccmd[250];
        FILE *pp;
        int stars = 0;
        int lines = 0;
        int drawn = 0;
        int x, y;
        float PEOffset;
        float PESpot;
        float decDrift;
        double rad;  //  telescope ra in degrees
        double rar;  //  telescope ra in radians
        double decr; //  telescope dec in radians;
        int nwidth = 0, nheight = 0;

        double timesince;
        time_t now;
        time(&now);

        //  Lets figure out where we are on the pe curve
        timesince = difftime(now, RunStart);
        //  This is our spot in the curve
        PESpot = timesince / PEPeriod;
        //  Now convert to radians
        PESpot = PESpot * 2.0 * 3.14159;

        PEOffset = PEMax * std::sin(PESpot);
        //fprintf(stderr,"PEOffset = %4.2f arcseconds timesince %4.2f\n",PEOffset,timesince);
        PEOffset = PEOffset / 3600; //  convert to degrees
        //PeOffset=PeOffset/15;       //  ra is in h:mm

        //  Start by clearing the frame buffer
        memset(targetChip->getFrameBuffer(), 0, targetChip->getFrameBufferSize());

        //  Spin up a set of plate constants that will relate
        //  ra/dec of stars, to our fictitious ccd layout

        //  to account for various rotations etc
        //  we should spin up some plate constants here
        //  then we can use these constants to rotate and offset
        //  the standard co-ordinates on each star for drawing
        //  a ccd frame;
        double pa, pb, pc, pd, pe, pf;
        // Pixels per radian
        double pprx, ppry;
        // Scale in arcsecs per pixel
        double Scalex;
        double Scaley;
        // CCD width in pixels
        double ccdW = targetChip->getXRes();

        // Pixels per radian
        pprx = targetFocalLength / targetChip->getPixelSizeX() * 1000;
        ppry = targetFocalLength / targetChip->getPixelSizeY() * 1000;

        //  we do a simple scale for x and y locations
        //  based on the focal length and pixel size
        //  focal length in mm, pixels in microns
        // JM: 2015-03-17: Using a simpler formula, Scalex and Scaley are in arcsecs/pixel
        Scalex = (targetChip->getPixelSizeX() / targetFocalLength) * 206.3;
        Scaley = (targetChip->getPixelSizeY() / targetFocalLength) * 206.3;

        DEBUGF(
            INDI::Logger::DBG_DEBUG,
            "pprx: %g pixels per radian ppry: %g pixels per radian ScaleX: %g arcsecs/pixel ScaleY: %g arcsecs/pixel",
            pprx, ppry, Scalex, Scaley);

        double theta = rotationCW + 270;
        if (theta > 360)
            theta -= 360;
        else if (theta < -360)
            theta += 360;

        // JM: 2015-03-17: Next we do a rotation assuming CW for angle theta
        pa = pprx * cos(theta * M_PI / 180.0);
        pb = ppry * sin(theta * M_PI / 180.0);

        pd = pprx * -sin(theta * M_PI / 180.0);
        pe = ppry * cos(theta * M_PI / 180.0);

        nwidth = targetChip->getXRes();
        pc     = nwidth / 2;

        nheight = targetChip->getYRes();
        pf      = nheight / 2;

        ImageScalex = Scalex;
        ImageScaley = Scaley;

        if (!usePE)
        {
            raPE  = RA;
            decPE = Dec;

            ln_equ_posn epochPos { 0, 0 }, J2000Pos { 0, 0 };

            epochPos.ra  = raPE * 15.0;
            epochPos.dec = decPE;

            // Convert from JNow to J2000
            ln_get_equ_prec2(&epochPos, ln_get_julian_from_sys(), JD2000, &J2000Pos);

            raPE  = J2000Pos.ra / 15.0;
            decPE = J2000Pos.dec;
        }

        //  calc this now, we will use it a lot later
        rad = raPE * 15.0;
        rar = rad * 0.0174532925;
        //  offsetting the dec by the guide head offset
        float cameradec;
        cameradec = decPE + OAGoffset / 60;
        decr      = cameradec * 0.0174532925;

        decDrift = (polarDrift * polarError * cos(decr)) / 3.81;

        // Add declination drift, if any.
        decr += decDrift / 3600.0 * 0.0174532925;

        //fprintf(stderr,"decPE %7.5f  cameradec %7.5f  CenterOffsetDec %4.4f\n",decPE,cameradec,decr);
        //  now lets calculate the radius we need to fetch
        float radius;

        radius = sqrt((Scalex * Scalex * targetChip->getXRes() / 2.0 * targetChip->getXRes() / 2.0) +
                      (Scaley * Scaley * targetChip->getYRes() / 2.0 * targetChip->getYRes() / 2.0));
        //  we have radius in arcseconds now
        radius = radius / 60; //  convert to arcminutes

        DEBUGF(INDI::Logger::DBG_DEBUG, "Lookup radius %4.2f", radius);

        //  A saturationmag star saturates in one second
        //  and a limitingmag produces a one adu level in one second
        //  solve for zero point and system gain

        k = (saturationmag - limitingmag) / ((-2.5 * log(maxval)) - (-2.5 * log(1.0 / 2.0)));
        z = saturationmag - k * (-2.5 * log(maxval));
        //z=z+saturationmag;

        //IDLog("K=%4.2f  Z=%4.2f\n",k,z);

        //  Should probably do some math here to figure out the dimmest
        //  star we can see on this exposure
        //  and only fetch to that magnitude
        //  for now, just use the limiting mag number with some room to spare
        float lookuplimit = limitingmag;

        if (radius > 60)
            lookuplimit = 11;

        //  if this is a light frame, we need a star field drawn
        INDI::CCDChip::CCD_FRAME ftype = targetChip->getFrameType();

        if (ftype == INDI::CCDChip::LIGHT_FRAME)
        {
            AutoCNumeric locale;

            //sprintf(gsccmd,"gsc -c %8.6f %+8.6f -r 120 -m 0 9.1",rad+PEOffset,decPE);
            sprintf(gsccmd, "gsc -c %8.6f %+8.6f -r %4.1f -m 0 %4.2f -n 3000", rad + PEOffset, cameradec, radius,
                    lookuplimit);
            DEBUGF(INDI::Logger::DBG_DEBUG, "%s", gsccmd);
            pp = popen(gsccmd, "r");
            if (pp != nullptr)
            {
                char line[256];
                while (fgets(line, 256, pp) != nullptr)
                {
                    //fprintf(stderr,"%s",line);

                    //  ok, lets parse this line for specifcs we want
                    char id[20];
                    char plate[6];
                    char ob[6];
                    float mag;
                    float mage;
                    float ra;
                    float dec;
                    float pose;
                    int band;
                    float dist;
                    int dir;
                    int c;
                    int rc;

                    rc = sscanf(line, "%10s %f %f %f %f %f %d %d %4s %2s %f %d", id, &ra, &dec, &pose, &mag, &mage,
                                &band, &c, plate, ob, &dist, &dir);
                    //fprintf(stderr,"Parsed %d items\n",rc);
                    if (rc == 12)
                    {
                        lines++;
                        //if(c==0) {
                        stars++;
                        //fprintf(stderr,"%s %8.4f %8.4f %5.2f %5.2f %d\n",id,ra,dec,mag,dist,dir);

                        //  Convert the ra/dec to standard co-ordinates
                        double sx;    //  standard co-ords
                        double sy;    //
                        double srar;  //  star ra in radians
                        double sdecr; //  star dec in radians;
                        double ccdx;
                        double ccdy;

                        //fprintf(stderr,"line %s",line);
                        //fprintf(stderr,"parsed %6.5f %6.5f\n",ra,dec);

                        srar  = ra * 0.0174532925;
                        sdecr = dec * 0.0174532925;
                        //  Handbook of astronomical image processing
                        //  page 253
                        //  equations 9.1 and 9.2
                        //  convert ra/dec to standard co-ordinates

                        sx = cos(sdecr) * sin(srar - rar) /
                             (cos(decr) * cos(sdecr) * cos(srar - rar) + sin(decr) * sin(sdecr));
                        sy = (sin(decr) * cos(sdecr) * cos(srar - rar) - cos(decr) * sin(sdecr)) /
                             (cos(decr) * cos(sdecr) * cos(srar - rar) + sin(decr) * sin(sdecr));

                        //  now convert to pixels
                        ccdx = pa * sx + pb * sy + pc;
                        ccdy = pd * sx + pe * sy + pf;

                        // Invert horizontally
                        ccdx = ccdW - ccdx;

                        rc = DrawImageStar(targetChip, mag, ccdx, ccdy, ExposureTime);
                        drawn += rc;
                        if (rc == 1)
                        {
                            //DEBUGF(INDI::Logger::DBG_DEBUG, "star %s scope %6.4f %6.4f star %6.4f %6.4f ccd %6.2f %6.2f",id,rad,decPE,ra,dec,ccdx,ccdy);
                            //DEBUGF(INDI::Logger::DBG_DEBUG, "star %s ccd %6.2f %6.2f",id,ccdx,ccdy);
                        }
                    }
                }
                pclose(pp);
            }
            else
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Error looking up stars, is gsc installed with appropriate environment variables set ??");
            }
            if (drawn == 0)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Got no stars, is gsc installed with appropriate environment variables set ??");
            }
        }
        //fprintf(stderr,"Got %d stars from %d lines drew %d\n",stars,lines,drawn);

        //  now we need to add background sky glow, with vignetting
        //  this is essentially the same math as drawing a dim star with
        //  fwhm equivalent to the full field of view

        if (ftype == INDI::CCDChip::LIGHT_FRAME || ftype == INDI::CCDChip::FLAT_FRAME)
        {
            float skyflux;
            //  calculate flux from our zero point and gain values
            float glow = skyglow;

            if (ftype == INDI::CCDChip::FLAT_FRAME)
            {
                //  Assume flats are done with a diffuser
                //  in broad daylight, so, the sky magnitude
                //  is much brighter than at night
                glow = skyglow / 10;
            }

            //fprintf(stderr,"Using glow %4.2f\n",glow);

            skyflux = pow(10, ((glow - z) * k / -2.5));
            //  ok, flux represents one second now
            //  scale up linearly for exposure time
            skyflux = skyflux * ExposureTime;
            //IDLog("SkyFlux = %g ExposureRequest %g\n",skyflux,ExposureTime);

            unsigned short *pt;

            pt = (uint16_t *)targetChip->getFrameBuffer();

            nheight = targetChip->getSubH();
            nwidth  = targetChip->getSubW();

            for (int y = 0; y < nheight; y++)
            {
                for (int x = 0; x < nwidth; x++)
                {
                    float dc; //  distance from center
                    float fp; //  flux this pixel;
                    float sx, sy;
                    float vig;

                    sx = nwidth / 2 - x;
                    sy = nheight / 2 - y;

                    vig = nwidth;
                    vig = vig * ImageScalex;
                    //  need to make this account for actual pixel size
                    dc = std::sqrt(sx * sx * ImageScalex * ImageScalex + sy * sy * ImageScaley * ImageScaley);
                    //  now we have the distance from center, in arcseconds
                    //  now lets plot a gaussian falloff to the edges
                    //
                    float fa;
                    fa = exp(-2.0 * 0.7 * (dc * dc) / vig / vig);

                    //  get the current value
                    fp = pt[0];

                    //  Add the sky glow
                    fp += skyflux;

                    //  now scale it for the vignetting
                    fp = fa * fp;

                    //  clamp to limits
                    if (fp > maxval)
                        fp = maxval;
                    if (fp > maxpix)
                        maxpix = fp;
                    if (fp < minpix)
                        minpix = fp;
                    //  and put it back
                    pt[0] = fp;
                    pt++;
                }
            }
        }

        //  Now we add some bias and read noise
        int subX = targetChip->getSubX();
        int subY = targetChip->getSubY();
        int subW = targetChip->getSubW() + subX;
        int subH = targetChip->getSubH() + subY;

        for (x = subX; x < subW; x++)
        {
            for (y = subY; y < subH; y++)
            {
                int noise;

                noise = random();
                noise = noise % maxnoise; //

                //IDLog("noise is %d\n", noise);
                AddToPixel(targetChip, x, y, bias + noise);
            }
        }
    }
    else
    {
        testvalue++;
        if (testvalue > 255)
            testvalue = 0;
        val = testvalue;

        int nbuf = targetChip->getSubW() * targetChip->getSubH();

        for (int x = 0; x < nbuf; x++)
        {
            *ptr = val++;
            ptr++;
        }
    }    

    return 0;
}

int CCDSim::DrawImageStar(INDI::CCDChip *targetChip, float mag, float x, float y, float ExposureTime)
{
    //float d;
    //float r;
    int sx, sy;
    int drew     = 0;
    int boxsizex = 5;
    int boxsizey = 5;
    float flux;

    int subX = targetChip->getSubX();
    int subY = targetChip->getSubY();
    int subW = targetChip->getSubW() + subX;
    int subH = targetChip->getSubH() + subY;

    if ((x < subX) || (x > subW || (y < subY) || (y > subH)))
    {
        //  this star is not on the ccd frame anyways
        return 0;
    }

    //  calculate flux from our zero point and gain values
    flux = pow(10, ((mag - z) * k / -2.5));

    //  ok, flux represents one second now
    //  scale up linearly for exposure time
    flux = flux * ExposureTime;

    float qx;
    //  we need a box size that gives a radius at least 3 times fwhm
    qx       = seeing / ImageScalex;
    qx       = qx * 3;
    boxsizex = (int)qx;
    boxsizex++;
    qx       = seeing / ImageScaley;
    qx       = qx * 3;
    boxsizey = (int)qx;
    boxsizey++;

    //IDLog("BoxSize %d %d\n",boxsizex,boxsizey);

    for (sy = -boxsizey; sy <= boxsizey; sy++)
    {
        for (sx = -boxsizey; sx <= boxsizey; sx++)
        {
            int rc;
            float dc; //  distance from center
            float fp; //  flux this pixel;

            //  need to make this account for actual pixel size
            dc = std::sqrt(sx * sx * ImageScalex * ImageScalex + sy * sy * ImageScaley * ImageScaley);
            //  now we have the distance from center, in arcseconds
            //  This should be gaussian, but, for now we'll just go with
            //  a simple linear function
            float fa = exp(-2.0 * 0.7 * (dc * dc) / seeing / seeing);

            fp = fa * flux;

            if (fp < 0)
                fp = 0;

            rc = AddToPixel(targetChip, x + sx, y + sy, fp);
            if (rc != 0)
                drew = 1;
        }
    }
    return drew;
}

int CCDSim::AddToPixel(INDI::CCDChip *targetChip, int x, int y, int val)
{
    int nwidth  = targetChip->getSubW();
    int nheight = targetChip->getSubH();

    x -= targetChip->getSubX();
    y -= targetChip->getSubY();

    int drew = 0;
    if (x >= 0)
    {
        if (x < nwidth)
        {
            if (y >= 0)
            {
                if (y < nheight)
                {
                    unsigned short *pt;
                    int newval;
                    drew++;

                    pt = (uint16_t *)targetChip->getFrameBuffer();

                    pt += (y * nwidth);
                    pt += x;
                    newval = pt[0];
                    newval += val;
                    if (newval > maxval)
                        newval = maxval;
                    if (newval > maxpix)
                        maxpix = newval;
                    if (newval < minpix)
                        minpix = newval;
                    pt[0] = newval;
                }
            }
        }
    }
    return drew;
}

IPState CCDSim::GuideNorth(float v)
{
    float c;

    c     = v / 1000 * GuideRate; //
    c     = c / 3600;
    decPE = decPE + c;

    return IPS_OK;
}

IPState CCDSim::GuideSouth(float v)
{
    float c;

    c     = v / 1000 * GuideRate; //
    c     = c / 3600;
    decPE = decPE - c;

    return IPS_OK;
}

IPState CCDSim::GuideEast(float v)
{
    float c;

    c    = v / 1000 * GuideRate;
    c    = c / 3600.0 / 15.0;
    c    = c / (cos(decPE * 0.0174532925));
    raPE = raPE + c;

    return IPS_OK;
}

IPState CCDSim::GuideWest(float v)
{
    float c;

    c    = v / 1000 * GuideRate; //
    c    = c / 3600.0 / 15.0;
    c    = c / (cos(decPE * 0.0174532925));
    raPE = raPE - c;

    return IPS_OK;
}

bool CCDSim::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        if (strcmp(name, FilterNameTP->name) == 0)
        {
            INDI::FilterInterface::processText(dev, name, texts, names, n);
            return true;
        }
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool CCDSim::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    //IDLog("INDI::CCD::ISNewNumber %s\n",name);
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here

        //IDLog("CCDSim::ISNewNumber %s\n",name);
        if (strcmp(name, "SIMULATOR_SETTINGS") == 0)
        {
            IUUpdateNumber(SimulatorSettingsNV, values, names, n);
            SimulatorSettingsNV->s = IPS_OK;

            //  Reset our parameters now
            SetupParms();
            IDSetNumber(SimulatorSettingsNV, nullptr);

            //IDLog("Frame set to %4.0f,%4.0f %4.0f x %4.0f\n",CcdFrameN[0].value,CcdFrameN[1].value,CcdFrameN[2].value,CcdFrameN[3].value);
            //seeing=SimulatorSettingsN[0].value;
            return true;
        }

        if (strcmp(name, FilterSlotNP.name) == 0)
        {
            INDI::FilterInterface::processNumber(dev, name, values, names, n);
            return true;
        }
    }
    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool CCDSim::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "ON_TIME_FACTOR") == 0)
        {
            //  client is telling us what to do with co-ordinate requests
            TimeFactorSV->s = IPS_OK;
            IUUpdateSwitch(TimeFactorSV, states, names, n);
            //  Update client display
            IDSetSwitch(TimeFactorSV, nullptr);

            if (TimeFactorS[0].s == ISS_ON)
            {
                //IDLog("CCDSim:: Time Factor 1\n");
                TimeFactor = 1;
            }
            if (TimeFactorS[1].s == ISS_ON)
            {
                //IDLog("CCDSim:: Time Factor 0.1\n");
                TimeFactor = 0.1;
            }
            if (TimeFactorS[2].s == ISS_ON)
            {
                //IDLog("CCDSim:: Time Factor 0.01\n");
                TimeFactor = 0.01;
            }

            return true;
        }
    }

    if (strcmp(name, CoolerSP.name) == 0)
    {
        IUUpdateSwitch(&CoolerSP, states, names, n);

        if (CoolerS[0].s == ISS_ON)
            CoolerSP.s = IPS_BUSY;
        else
        {
            CoolerSP.s         = IPS_IDLE;
            TemperatureRequest = 20;
            TemperatureNP.s    = IPS_BUSY;
        }

        IDSetSwitch(&CoolerSP, nullptr);

        return true;
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

void CCDSim::activeDevicesUpdated()
{
    IDSnoopDevice(ActiveDeviceT[0].text, "EQUATORIAL_PE");
    IDSnoopDevice(ActiveDeviceT[1].text, "FWHM");

    strncpy(EqPENP.device, ActiveDeviceT[0].text, MAXINDIDEVICE);
    strncpy(FWHMNP.device, ActiveDeviceT[1].text, MAXINDIDEVICE);
}

bool CCDSim::ISSnoopDevice(XMLEle *root)
{
    if (IUSnoopNumber(root, &FWHMNP) == 0)
    {
        seeing = FWHMNP.np[0].value;

        //IDLog("CCD Simulator: New FWHM value of %g\n", seeing);
        return true;
    }

    // We try to snoop EQPEC first, if not found, we snoop regular EQNP
    if (IUSnoopNumber(root, &EqPENP) == 0)
    {
        double newra, newdec;
        newra  = EqPEN[0].value;
        newdec = EqPEN[1].value;
        if ((newra != raPE) || (newdec != decPE))
        {
            ln_equ_posn epochPos { 0, 0 }, J2000Pos { 0, 0 };
            epochPos.ra  = newra * 15.0;
            epochPos.dec = newdec;

            ln_get_equ_prec2(&epochPos, ln_get_julian_from_sys(), JD2000, &J2000Pos);

            raPE  = J2000Pos.ra / 15.0;
            decPE = J2000Pos.dec;

            usePE = true;

            DEBUGF(INDI::Logger::DBG_DEBUG, "raPE %g  decPE %g Snooped raPE %g  decPE %g", raPE, decPE, newra, newdec);

            return true;
        }
    }

    return INDI::CCD::ISSnoopDevice(root);
}

bool CCDSim::saveConfigItems(FILE *fp)
{
    // Save CCD Config
    INDI::CCD::saveConfigItems(fp);

    // Save Filter Wheel Config
    INDI::FilterInterface::saveConfigItems(fp);

    // Save CCD Simulator Config
    IUSaveConfigNumber(fp, SimulatorSettingsNV);
    IUSaveConfigSwitch(fp, TimeFactorSV);

    return true;
}

bool CCDSim::SelectFilter(int f)
{
    CurrentFilter = f;
    SelectFilterDone(f);
    return true;
}

int CCDSim::QueryFilter()
{
    return CurrentFilter;
}

bool CCDSim::StartStreaming()
{
    ExposureRequest = 1.0 / Streamer->getTargetFPS();
    pthread_mutex_lock(&condMutex);
    streamPredicate = 1;
    pthread_mutex_unlock(&condMutex);
    pthread_cond_signal(&cv);

    return true;
}

bool CCDSim::StopStreaming()
{
    pthread_mutex_lock(&condMutex);
    streamPredicate = 0;
    pthread_mutex_unlock(&condMutex);
    pthread_cond_signal(&cv);

    return true;
}

bool CCDSim::UpdateCCDFrame(int x, int y, int w, int h)
{
    long bin_width  = w / PrimaryCCD.getBinX();
    long bin_height = h / PrimaryCCD.getBinY();

    bin_width  = bin_width - (bin_width % 2);
    bin_height = bin_height - (bin_height % 2);

    Streamer->setSize(bin_width, bin_height);

    return INDI::CCD::UpdateCCDFrame(x,y,w,h);
}

bool CCDSim::UpdateCCDBin(int hor, int ver)
{
    if (hor == 3 || ver == 3)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "3x3 binning is not supported.");
        return false;
    }

    long bin_width  = PrimaryCCD.getSubW() / hor;
    long bin_height = PrimaryCCD.getSubH() / ver;

    bin_width  = bin_width - (bin_width % 2);
    bin_height = bin_height - (bin_height % 2);

    Streamer->setSize(bin_width, bin_height);

    return INDI::CCD::UpdateCCDBin(hor,ver);
}

void *CCDSim::streamVideoHelper(void *context)
{
    return ((CCDSim *)context)->streamVideo();
}

void *CCDSim::streamVideo()
{
    struct itimerval tframe1, tframe2;
    double s1, s2, deltas;

    while (true)
    {
        pthread_mutex_lock(&condMutex);

        while (streamPredicate == 0)
        {
            pthread_cond_wait(&cv, &condMutex);
            ExposureRequest = 1.0 / Streamer->getTargetFPS();
        }

        if (terminateThread)
            break;

        // release condMutex
        pthread_mutex_unlock(&condMutex);

        // Simulate exposure time
        //usleep(ExposureRequest*1e5);

        // 16 bit
        DrawCcdFrame(&PrimaryCCD);

        PrimaryCCD.binFrame();

        getitimer(ITIMER_REAL, &tframe1);

        s1 = ((double)tframe1.it_value.tv_sec) + ((double)tframe1.it_value.tv_usec / 1e6);
        s2 = ((double)tframe2.it_value.tv_sec) + ((double)tframe2.it_value.tv_usec / 1e6);
        deltas = fabs(s2 - s1);

        if (deltas < ExposureRequest)
            usleep(fabs(ExposureRequest-deltas)*1e6);

        uint32_t size = PrimaryCCD.getFrameBufferSize() / (PrimaryCCD.getBinX()*PrimaryCCD.getBinY());
        Streamer->newFrame(PrimaryCCD.getFrameBuffer(), size);

        getitimer(ITIMER_REAL, &tframe2);
    }

    pthread_mutex_unlock(&condMutex);
    return 0;
}
