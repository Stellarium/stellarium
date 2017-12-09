/* Copyright 2012 Geehalel (geehalel AT gmail DOT com) */
/* This file is part of the Skywatcher Protocol INDI driver.

    The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.

    2013-10-20: Fixed a few bugs and init/update properties issue (Jasem Mutlaq)
    2013-10-24: Use updateTime from new INDI framework (Jasem Mutlaq)
    2013-10-31: Added support for joysticks (Jasem Mutlaq)
    2013-11-01: Fixed issues with logger and Skywatcher's readout for InquireHighSpeedRatio.
*/

/* TODO */
/* HORIZONTAL_COORDS -> HORIZONTAL_COORD - OK */
/* DATE -> TIME_LST/LST and TIME_UTC/UTC - OK */
/*  Problem in time initialization using gettimeofday/gmtime: 1h after UTC on summer, because of DST ?? */
/* TELESCOPE_MOTION_RATE in arcmin/s */
/* use/snoop a GPS ??*/

#include "eqmod.h"

#include "mach_gettime.h"

#include <cmath>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <indicom.h>

#ifdef WITH_ALIGN
#include <alignment/DriverCommon.h> // For DBG_ALIGNMENT
using namespace INDI::AlignmentSubsystem;
#endif

#include <libnova/sidereal_time.h>
#include <libnova/transform.h>
#include <libnova/utility.h>

#define DEVICE_NAME "EQMod Mount"

// We declare an auto pointer to EQMod.
std::unique_ptr<EQMod> eqmod(new EQMod());

#define GOTO_RATE      2        /* slew rate, degrees/s */
#define SLEW_RATE      0.5      /* slew rate, degrees/s */
#define FINE_SLEW_RATE 0.1      /* slew rate, degrees/s */
#define SID_RATE       0.004178 /* sidereal rate, degrees/s */

#define GOTO_LIMIT      5   /* Move at GOTO_RATE until distance from target is GOTO_LIMIT degrees */
#define SLEW_LIMIT      2   /* Move at SLEW_LIMIT until distance from target is SLEW_LIMIT degrees */
#define FINE_SLEW_LIMIT 0.5 /* Move at FINE_SLEW_RATE until distance from target is FINE_SLEW_LIMIT degrees */

#define POLLMS 1000

#define GOTO_ITERATIVE_LIMIT 5 /* Max GOTO Iterations */
#define RAGOTORESOLUTION     5 /* GOTO Resolution in arcsecs */
#define DEGOTORESOLUTION     5 /* GOTO Resolution in arcsecs */

/* Preset Slew Speeds */
#define SLEWMODES 11
double slewspeeds[SLEWMODES - 1] = { 1.0, 2.0, 4.0, 8.0, 32.0, 64.0, 128.0, 600.0, 700.0, 800.0 };
double defaultspeed              = 64.0;

#define RA_AXIS     0
#define DEC_AXIS    1
#define GUIDE_NORTH 0
#define GUIDE_SOUTH 1
#define GUIDE_WEST  0
#define GUIDE_EAST  1

int DBG_SCOPE_STATUS;
int DBG_COMM;
int DBG_MOUNT;

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec)
    {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000)
    {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
    result->tv_sec  = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

void ISGetProperties(const char *dev)
{
    eqmod->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    eqmod->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    eqmod->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    eqmod->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
#ifdef WITH_ALIGN
    eqmod->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
#else
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
#endif
}
void ISSnoopDevice(XMLEle *root)
{
    eqmod->ISSnoopDevice(root);
}

EQMod::EQMod()
{
    //ctor
    setVersion(EQMOD_VERSION_MAJOR, EQMOD_VERSION_MINOR);
    // Do not define dynamic properties on startup, and do not delete them from memory
    setDynamicPropertiesBehavior(false, false);
    currentRA            = 0;
    currentDEC           = 90;
    gotoparams.completed = true;
    last_motion_ns       = -1;
    last_motion_ew       = -1;

    DBG_SCOPE_STATUS = INDI::Logger::getInstance().addDebugLevel("Scope Status", "SCOPE");
    DBG_COMM         = INDI::Logger::getInstance().addDebugLevel("Serial Port", "COMM");
    DBG_MOUNT        = INDI::Logger::getInstance().addDebugLevel("Verbose Mount", "MOUNT");

    mount = new Skywatcher(this);

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT | TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION
                           | TELESCOPE_HAS_PIER_SIDE | TELESCOPE_HAS_TRACK_RATE | TELESCOPE_HAS_TRACK_MODE | TELESCOPE_CAN_CONTROL_TRACK,
                           SLEWMODES);

    RAInverted = DEInverted = false;
    bzero(&syncdata, sizeof(syncdata));
    bzero(&syncdata2, sizeof(syncdata2));

#ifdef WITH_ALIGN_GEEHALEL
    align = new Align(this);
#endif

    simulator = new EQModSimulator(this);

#ifdef WITH_SCOPE_LIMITS
    horizon = new HorizonLimits(this);
#endif

    /* initialize time */
    tzset();
    gettimeofday(&lasttimeupdate, NULL); // takes care of DST
    gmtime_r(&lasttimeupdate.tv_sec, &utc);
    lndate.seconds = utc.tm_sec + ((double)lasttimeupdate.tv_usec / 1000000);
    lndate.minutes = utc.tm_min;
    lndate.hours   = utc.tm_hour;
    lndate.days    = utc.tm_mday;
    lndate.months  = utc.tm_mon + 1;
    lndate.years   = utc.tm_year + 1900;
    get_utc_time(&lastclockupdate);
    /* initialize random seed: */
    srand(time(NULL));
    // Others
    AutohomeState      = AUTO_HOME_IDLE;
    restartguideRAPPEC = false;
    restartguideDEPPEC = false;
}

EQMod::~EQMod()
{
    //dtor
    if (mount)
        delete mount;
    mount = NULL;
}

#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
bool EQMod::isStandardSync()
{
    return (strcmp(IUFindOnSwitch(AlignSyncModeSP)->name, "ALIGNSTANDARDSYNC") == 0);
}
#endif

void EQMod::setStepperSimulation(bool enable)
{
    if ((enable && !isSimulation()) || (!enable && isSimulation()))
    {
        mount->setSimulation(enable);
        if (not simulator->updateProperties(enable))
            DEBUG(INDI::Logger::DBG_WARNING, "setStepperSimulator: Disable/Enable error");
    }
    INDI::Telescope::setSimulation(enable);
}

const char *EQMod::getDefaultName()
{
    return (char *)DEVICE_NAME;
}

double EQMod::getLongitude()
{
    return (IUFindNumber(&LocationNP, "LONG")->value);
}

double EQMod::getLatitude()
{
    return (IUFindNumber(&LocationNP, "LAT")->value);
}

double EQMod::getJulianDate()
{
    /*
  struct timeval currenttime, difftime;
  double usecs;
  gettimeofday(&currenttime, NULL);
  if (timeval_subtract(&difftime, &currenttime, &lasttimeupdate) == -1)
    return juliandate;
  */
    struct timespec currentclock, diffclock;
    double nsecs;
    get_utc_time(&currentclock);
    diffclock.tv_sec  = currentclock.tv_sec - lastclockupdate.tv_sec;
    diffclock.tv_nsec = currentclock.tv_nsec - lastclockupdate.tv_nsec;
    while (diffclock.tv_nsec > 1000000000)
    {
        diffclock.tv_sec++;
        diffclock.tv_nsec -= 1000000000;
    }
    while (diffclock.tv_nsec < 0)
    {
        diffclock.tv_sec--;
        diffclock.tv_nsec += 1000000000;
    }
    //IDLog("Get Julian; ln_date was %02d:%02d:%.9f\n", lndate.hours, lndate.minutes, lndate.seconds);
    //IDLog("Clocks last: %d secs %d nsecs current: %d secs %d nsecs\n", lastclockupdate.tv_sec,  lastclockupdate.tv_nsec, currentclock.tv_sec,  currentclock.tv_nsec);
    //IDLog("Diff %d secs %d nsecs\n", diffclock.tv_sec,  diffclock.tv_nsec);
    //IDLog("Diff %d %d\n", difftime.tv_sec,  difftime.tv_usec);
    //lndate.seconds += (difftime.tv_sec + (difftime.tv_usec / 1000000));
    //usecs=lndate.seconds - floor(lndate.seconds);
    lndate.seconds += (diffclock.tv_sec + ((double)diffclock.tv_nsec / 1000000000.0));
    nsecs        = lndate.seconds - floor(lndate.seconds);
    utc.tm_sec   = lndate.seconds;
    utc.tm_isdst = -1; // let mktime find if DST already in effect in utc
    //IDLog("Get julian: setting UTC secs to %f", utc.tm_sec);
    mktime(&utc); // normalize time
    //IDLog("Get Julian; UTC is now %s", asctime(&utc));
    ln_get_date_from_tm(&utc, &lndate);
    //IDLog("Get Julian; ln_date is now %02d:%02d:%.9f\n", lndate.hours, lndate.minutes, lndate.seconds);
    //lndate.seconds+=usecs;
    //lasttimeupdate = currenttime;
    lndate.seconds += nsecs;
    //IDLog("     ln_date with nsecs %02d:%02d:%.9f\n", lndate.hours, lndate.minutes, lndate.seconds);
    lastclockupdate = currentclock;
    juliandate      = ln_get_julian_day(&lndate);
    //IDLog("julian diff: %g\n", juliandate - ln_get_julian_from_sys());
    return juliandate;
    //return ln_get_julian_from_sys();
}

double EQMod::getLst(double jd, double lng)
{
    double lst;
    //lst=ln_get_mean_sidereal_time(jd);
    lst = ln_get_apparent_sidereal_time(jd);
    lst += (lng / 15.0);
    lst = range24(lst);
    return lst;
}

bool EQMod::initProperties()
{
    /* Make sure to init parent properties first */
    INDI::Telescope::initProperties();

    loadProperties();

    for (int i = 0; i < SlewRateSP.nsp - 1; i++)
    {
        sprintf(SlewRateSP.sp[i].label, "%.fx", slewspeeds[i]);
        SlewRateSP.sp[i].aux = (void *)&slewspeeds[i];
    }

    // Since last item is NOT maximum (but custom), let's set item before custom to SLEWMAX
    strncpy(SlewRateSP.sp[SlewRateSP.nsp - 2].name, "SLEW_MAX", MAXINDINAME);

    strncpy(SlewRateSP.sp[SlewRateSP.nsp - 1].name, "SLEWCUSTOM", MAXINDINAME);
    strncpy(SlewRateSP.sp[SlewRateSP.nsp - 1].label, "Custom", MAXINDILABEL);

    AddTrackMode("TRACK_SIDEREAL", "Sidereal", true);
    AddTrackMode("TRACK_SOLAR", "Solar");
    AddTrackMode("TRACK_LUNAR", "Lunar");
    AddTrackMode("TRACK_CUSTOM", "Custom");

    SetParkDataType(PARK_RA_DEC_ENCODER);

    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);
#ifdef WITH_ALIGN
    InitAlignmentProperties(this);

    // Force the alignment system to always be on
    getSwitch("ALIGNMENT_SUBSYSTEM_ACTIVE")->sp[0].s = ISS_ON;

#endif

    addAuxControls();
    return true;
}

void EQMod::ISGetProperties(const char *dev)
{
    INDI::Telescope::ISGetProperties(dev);

    if (isConnected())
    {
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
        defineNumber(SlewSpeedsNP);
        defineNumber(GuideRateNP);
        defineText(MountInformationTP);
        defineNumber(SteppersNP);
        defineNumber(CurrentSteppersNP);
        defineNumber(PeriodsNP);
        defineNumber(JulianNP);
        defineNumber(TimeLSTNP);
        defineLight(RAStatusLP);
        defineLight(DEStatusLP);
        defineSwitch(HemisphereSP);
        defineNumber(HorizontalCoordNP);
        defineSwitch(ReverseDECSP);
        defineNumber(StandardSyncNP);
        defineNumber(StandardSyncPointNP);
        defineNumber(SyncPolarAlignNP);
        defineSwitch(SyncManageSP);
        defineNumber(BacklashNP);
        defineSwitch(UseBacklashSP);
        defineSwitch(TrackDefaultSP);
        defineSwitch(ST4GuideRateNSSP);
        defineSwitch(ST4GuideRateWESP);
#if defined WITH_ALIGN && defined WITH_ALIGN_GEEHALEL
        defineSwitch(&AlignMethodSP);
#endif
#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
        defineSwitch(AlignSyncModeSP);
#endif
        if (mount->HasHomeIndexers())
        {
            defineSwitch(AutoHomeSP);
        }

        if (mount->HasAuxEncoders())
        {
            defineSwitch(AuxEncoderSP);
            defineNumber(AuxEncoderNP);
        }
        if (mount->HasPPEC())
        {
            defineSwitch(RAPPECTrainingSP);
            defineSwitch(RAPPECSP);
            defineSwitch(DEPPECTrainingSP);
            defineSwitch(DEPPECSP);
        }

#ifdef WITH_ALIGN_GEEHALEL
        if (align)
        {
            align->ISGetProperties();
        }
#endif

#ifdef WITH_SCOPE_LIMITS
        if (horizon)
        {
            horizon->ISGetProperties();
        }
#endif

        simulator->updateProperties(isSimulation());
    }
}

bool EQMod::loadProperties()
{
    buildSkeleton("indi_eqmod_sk.xml");

    GuideRateNP = getNumber("GUIDE_RATE");
    GuideRateN  = GuideRateNP->np;

    MountInformationTP = getText("MOUNTINFORMATION");
    SteppersNP         = getNumber("STEPPERS");
    CurrentSteppersNP  = getNumber("CURRENTSTEPPERS");
    PeriodsNP          = getNumber("PERIODS");
    JulianNP           = getNumber("JULIAN");
    TimeLSTNP          = getNumber("TIME_LST");
    RAStatusLP         = getLight("RASTATUS");
    DEStatusLP         = getLight("DESTATUS");
    SlewSpeedsNP       = getNumber("SLEWSPEEDS");
    HemisphereSP       = getSwitch("HEMISPHERE");
    TrackDefaultSP     = getSwitch("TELESCOPE_TRACK_DEFAULT");
    ReverseDECSP       = getSwitch("REVERSEDEC");

    HorizontalCoordNP   = getNumber("HORIZONTAL_COORD");
    StandardSyncNP      = getNumber("STANDARDSYNC");
    StandardSyncPointNP = getNumber("STANDARDSYNCPOINT");
    SyncPolarAlignNP    = getNumber("SYNCPOLARALIGN");
    SyncManageSP        = getSwitch("SYNCMANAGE");
    BacklashNP          = getNumber("BACKLASH");
    UseBacklashSP       = getSwitch("USEBACKLASH");
    AutoHomeSP          = getSwitch("AUTOHOME");
    AuxEncoderSP        = getSwitch("AUXENCODER");
    AuxEncoderNP        = getNumber("AUXENCODERVALUES");
    ST4GuideRateNSSP    = getSwitch("ST4_GUIDE_RATE_NS");
    ST4GuideRateWESP    = getSwitch("ST4_GUIDE_RATE_WE");
    RAPPECTrainingSP    = getSwitch("RA_PPEC_TRAINING");
    RAPPECSP            = getSwitch("RA_PPEC");
    DEPPECTrainingSP    = getSwitch("DE_PPEC_TRAINING");
    DEPPECSP            = getSwitch("DE_PPEC");
#ifdef WITH_ALIGN_GEEHALEL
    align->initProperties();
#endif
#if defined WITH_ALIGN && defined WITH_ALIGN_GEEHALEL
    IUFillSwitch(&AlignMethodS[0], "ALIGN_METHOD_EQMOD", "EQMod Align", ISS_ON);
    IUFillSwitch(&AlignMethodS[1], "ALIGN_METHOD_SUBSYSTEM", "Alignment Subsystem", ISS_OFF);
    IUFillSwitchVector(&AlignMethodSP, AlignMethodS, NARRAY(AlignMethodS), getDeviceName(), "ALIGN_METHOD",
                       "Align Method", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
#endif
#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
    AlignSyncModeSP = getSwitch("ALIGNSYNCMODE");
#endif

    simulator->initProperties();

    INDI::GuiderInterface::initGuiderProperties(this->getDeviceName(), MOTION_TAB);

#ifdef WITH_SCOPE_LIMITS
    if (horizon)
    {
        if (!horizon->initProperties())
            return false;
    }
#endif

    return true;
}

bool EQMod::updateProperties()
{
    INumber *latitude;

    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
        defineNumber(SlewSpeedsNP);
        defineNumber(GuideRateNP);
        defineText(MountInformationTP);
        defineNumber(SteppersNP);
        defineNumber(CurrentSteppersNP);
        defineNumber(PeriodsNP);
        defineNumber(JulianNP);
        defineNumber(TimeLSTNP);
        defineLight(RAStatusLP);
        defineLight(DEStatusLP);
        defineSwitch(HemisphereSP);
        defineNumber(HorizontalCoordNP);
        defineSwitch(ReverseDECSP);
        defineNumber(StandardSyncNP);
        defineNumber(StandardSyncPointNP);
        defineNumber(SyncPolarAlignNP);
        defineSwitch(SyncManageSP);
        defineNumber(BacklashNP);
        defineSwitch(UseBacklashSP);
        defineSwitch(TrackDefaultSP);
        defineSwitch(ST4GuideRateNSSP);
        defineSwitch(ST4GuideRateWESP);
#if defined WITH_ALIGN && defined WITH_ALIGN_GEEHALEL
        defineSwitch(&AlignMethodSP);
#endif
#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
        defineSwitch(AlignSyncModeSP);
#endif
        try
        {
            mount->InquireBoardVersion(MountInformationTP);

            if (isDebug())
            {
                for (int i = 0; i < MountInformationTP->ntp; i++)
                    DEBUGF(INDI::Logger::DBG_DEBUG, "Got Board Property %s: %s", MountInformationTP->tp[i].name,
                           MountInformationTP->tp[i].text);
            }

            mount->InquireRAEncoderInfo(SteppersNP);
            mount->InquireDEEncoderInfo(SteppersNP);
            if (isDebug())
            {
                for (int i = 0; i < SteppersNP->nnp; i++)
                    DEBUGF(INDI::Logger::DBG_DEBUG, "Got Encoder Property %s: %.0f", SteppersNP->np[i].label,
                           SteppersNP->np[i].value);
            }

            mount->InquireFeatures();
            //if (!strcmp(MountInformationTP->tp[0].text, "EQ8") || !strcmp(MountInformationTP->tp[0].text, "AZEQ6"))
            if (mount->HasHomeIndexers())
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Mount has home indexers. Enabling Autohome.");
                defineSwitch(AutoHomeSP);
            }

            if (mount->HasAuxEncoders())
            {
                defineSwitch(AuxEncoderSP);
                defineNumber(AuxEncoderNP);
                DEBUG(INDI::Logger::DBG_SESSION, "Mount has auxiliary encoders. Turning them off.");
                mount->TurnRAEncoder(false);
                mount->TurnDEEncoder(false);
            }
            if (mount->HasPPEC())
            {
                bool intraining, inppec;
                defineSwitch(RAPPECTrainingSP);
                defineSwitch(RAPPECSP);
                defineSwitch(DEPPECTrainingSP);
                defineSwitch(DEPPECSP);
                DEBUG(INDI::Logger::DBG_SESSION, "Mount has PPEC.");
                mount->GetRAPPECStatus(&intraining, &inppec);
                if (intraining)
                {
                    RAPPECTrainingSP->sp[0].s = ISS_OFF;
                    RAPPECTrainingSP->sp[1].s = ISS_ON;
                    RAPPECTrainingSP->s       = IPS_BUSY;
                    IDSetSwitch(RAPPECTrainingSP, NULL);
                }
                if (inppec)
                {
                    RAPPECSP->sp[0].s = ISS_OFF;
                    RAPPECSP->sp[1].s = ISS_ON;
                    RAPPECSP->s       = IPS_BUSY;
                    IDSetSwitch(RAPPECSP, NULL);
                }
                mount->GetDEPPECStatus(&intraining, &inppec);
                if (intraining)
                {
                    DEPPECTrainingSP->sp[0].s = ISS_OFF;
                    DEPPECTrainingSP->sp[1].s = ISS_ON;
                    DEPPECTrainingSP->s       = IPS_BUSY;
                    IDSetSwitch(DEPPECTrainingSP, NULL);
                }
                if (inppec)
                {
                    DEPPECSP->sp[0].s = ISS_OFF;
                    DEPPECSP->sp[1].s = ISS_ON;
                    DEPPECSP->s       = IPS_BUSY;
                    IDSetSwitch(DEPPECSP, NULL);
                }
            }

            mount->Init();

            zeroRAEncoder  = mount->GetRAEncoderZero();
            totalRAEncoder = mount->GetRAEncoderTotal();
            homeRAEncoder  = mount->GetRAEncoderHome();
            zeroDEEncoder  = mount->GetDEEncoderZero();
            totalDEEncoder = mount->GetDEEncoderTotal();
            homeDEEncoder  = mount->GetDEEncoderHome();

            parkRAEncoder = GetAxis1Park();
            parkDEEncoder = GetAxis2Park();

            latitude = IUFindNumber(&LocationNP, "LAT");
            if ((latitude) && (latitude->value < 0.0))
                SetSouthernHemisphere(true);
            else
                SetSouthernHemisphere(false);
        }
        catch (EQModError e)
        {
            return (e.DefaultHandleException(this));
        }
    }
    else
    {
        //if (MountInformationTP) {
        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);
        deleteProperty(GuideRateNP->name);
        deleteProperty(MountInformationTP->name);
        deleteProperty(SteppersNP->name);
        deleteProperty(CurrentSteppersNP->name);
        deleteProperty(PeriodsNP->name);
        deleteProperty(JulianNP->name);
        deleteProperty(TimeLSTNP->name);
        deleteProperty(RAStatusLP->name);
        deleteProperty(DEStatusLP->name);
        deleteProperty(SlewSpeedsNP->name);
        deleteProperty(HemisphereSP->name);
        deleteProperty(HorizontalCoordNP->name);
        deleteProperty(ReverseDECSP->name);
        deleteProperty(StandardSyncNP->name);
        deleteProperty(StandardSyncPointNP->name);
        deleteProperty(SyncPolarAlignNP->name);
        deleteProperty(SyncManageSP->name);
        deleteProperty(TrackDefaultSP->name);
        deleteProperty(BacklashNP->name);
        deleteProperty(UseBacklashSP->name);
        deleteProperty(ST4GuideRateNSSP->name);
        deleteProperty(ST4GuideRateWESP->name);
        //if (!strcmp(MountInformationTP->tp[0].text, "EQ8") || !strcmp(MountInformationTP->tp[0].text, "AZEQ6"))
        if (mount->HasHomeIndexers())
            deleteProperty(AutoHomeSP->name);
        if (mount->HasAuxEncoders())
        {
            deleteProperty(AuxEncoderSP->name);
            deleteProperty(AuxEncoderNP->name);
        }
        if (mount->HasPPEC())
        {
            deleteProperty(RAPPECTrainingSP->name);
            deleteProperty(RAPPECSP->name);
            deleteProperty(DEPPECTrainingSP->name);
            deleteProperty(DEPPECSP->name);
        }
#if defined WITH_ALIGN && defined WITH_ALIGN_GEEHALEL
        deleteProperty(AlignMethodSP.name);
#endif
#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
        deleteProperty(AlignSyncModeSP->name);
#endif
        //MountInformationTP=NULL;
        //}
    }
#ifdef WITH_ALIGN_GEEHALEL
    if (align)
    {
        if (!align->updateProperties())
            return false;
    }
#endif

#ifdef WITH_SCOPE_LIMITS
    if (horizon)
    {
        if (!horizon->updateProperties())
            return false;
    }
#endif

    simulator->updateProperties(isSimulation());

    return true;
}

bool EQMod::Handshake()
{
    try
    {
        mount->setPortFD(PortFD);
        mount->Handshake();
        // Mount initialisation is in updateProperties as it sets directly Indi properties which should be defined
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

#ifdef WITH_ALIGN
    // Set this according to mount type
    SetApproximateMountAlignmentFromMountType(EQUATORIAL);
#endif

    DEBUG(INDI::Logger::DBG_SESSION, "Successfully connected to EQMod Mount.");
    return true;
}

bool EQMod::Disconnect()
{
    if (isConnected())
    {
        try
        {
            mount->Disconnect();
        }
        catch (EQModError e)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error when disconnecting mount -> %s", e.message);
            return (false);
        }
        return INDI::Telescope::Disconnect();
    }
    else
        return false;
}

void EQMod::TimerHit()
{
    if (isConnected())
    {
        bool rc;

        rc = ReadScopeStatus();
        //IDLog("TrackState after read is %d\n",TrackState);
        if (rc == false)
        {
            // read was not good
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, NULL);
        }

        SetTimer(POLLMS);
    }
}

bool EQMod::ReadScopeStatus()
{
    //static struct timeval ltv;
    //struct timeval tv;
    //double dt=0;

    /* update elapsed time since last poll, don't presume exactly POLLMS */
    // gettimeofday (&tv, NULL);

    //if (ltv.tv_sec == 0 && ltv.tv_usec == 0)
    //  ltv = tv;

    //dt = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec)/1e6;
    //ltv = tv;
    //TODO use dt to track mount desynchronisation/inactivity?

    // Time
    double juliandate = 0;
    double lst = 0;
    //double datevalues[2];
    char hrlst[12];

    const char *datenames[] = { "LST", "JULIANDATE", "UTC" };
    double periods[2];
    const char *periodsnames[] = { "RAPERIOD", "DEPERIOD" };
    double horizvalues[2];
    const char *horiznames[2] = { "AZ", "ALT" };
    double steppervalues[2];
    const char *steppernames[] = { "RAStepsCurrent", "DEStepsCurrent" };

    juliandate = getJulianDate();
    lst        = getLst(juliandate, getLongitude());

    fs_sexa(hrlst, lst, 2, 360000);
    hrlst[11] = '\0';
    DEBUGF(DBG_SCOPE_STATUS, "Compute local time: lst=%2.8f (%s) - julian date=%8.8f", lst, hrlst, juliandate);

    IUUpdateNumber(TimeLSTNP, &lst, (char **)(datenames), 1);
    TimeLSTNP->s = IPS_OK;
    IDSetNumber(TimeLSTNP, NULL);

    IUUpdateNumber(JulianNP, &juliandate, (char **)(datenames + 1), 1);
    JulianNP->s = IPS_OK;
    IDSetNumber(JulianNP, NULL);

    char utcFormat[MAXINDINAME];
    strftime(utcFormat, MAXINDINAME, "%Y-%m-%dT%H:%M:%S", &utc);
    IUSaveText(IUFindText(&TimeTP, "UTC"), utcFormat);
    TimeTP.s = IPS_OK;
    IDSetText(&TimeTP, NULL);

    try
    {
        currentRAEncoder = mount->GetRAEncoder();
        currentDEEncoder = mount->GetDEEncoder();
        DEBUGF(DBG_SCOPE_STATUS, "Current encoders RA=%ld DE=%ld", currentRAEncoder, currentDEEncoder);
        EncodersToRADec(currentRAEncoder, currentDEEncoder, lst, &currentRA, &currentDEC, &currentHA);
        alignedRA    = currentRA;
        alignedDEC   = currentDEC;
        ghalignedRA  = currentRA;
        ghalignedDEC = currentDEC;
        bool aligned = false;
#ifdef WITH_ALIGN_GEEHALEL
        if (align)
        {
            align->GetAlignedCoords(syncdata, juliandate, &lnobserver, currentRA, currentDEC, &ghalignedRA,
                                    &ghalignedDEC);
            aligned = true;
        }
        //   else
#endif
#ifdef WITH_ALIGN
        const char *maligns[3] = { "ZENITH", "NORTH", "SOUTH" };
        struct ln_equ_posn RaDec;
        // Use HA/Dec as  telescope coordinate system
        RaDec.ra                     = ((lst - currentRA) * 360.0) / 24.0;
        RaDec.dec                    = currentDEC;
        TelescopeDirectionVector TDV = TelescopeDirectionVectorFromLocalHourAngleDeclination(RaDec);
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
               "Status: Mnt. Algnt. %s Date %lf encoders RA=%ld DE=%ld Telescope RA %lf DEC %lf",
               maligns[GetApproximateMountAlignment()], juliandate, currentRAEncoder, currentDEEncoder, currentRA,
                currentDEC);
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, " Direction RA(deg.)  %lf DEC %lf TDV(x %lf y %lf z %lf)",
               RaDec.ra, RaDec.dec, TDV.x, TDV.y, TDV.z);
        aligned = true;
        if ((GetAlignmentDatabase().size() < 2) || (!TransformTelescopeToCelestial(TDV, alignedRA, alignedDEC)))
        {
            //if (!TransformTelescopeToCelestial( TDV, alignedRA, alignedDEC)) {
            aligned = false;
            DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
                   "Failed TransformTelescopeToCelestial: Scope RA=%g Scope DE=%f, Aligned RA=%f DE=%f", currentRA,
                   currentDEC, alignedRA, alignedDEC);
        }
        else
        {
            DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
                   "TransformTelescopeToCelestial: Scope RA=%f Scope DE=%f, Aligned RA=%f DE=%f", currentRA, currentDEC,
                   alignedRA, alignedDEC);
        }
#endif
        if (!aligned && (syncdata.lst != 0.0))
        {
            DEBUGF(DBG_SCOPE_STATUS, "Aligning with last sync delta RA %g DE %g", syncdata.deltaRA, syncdata.deltaDEC);
            // should check values are in range!
            alignedRA += syncdata.deltaRA;
            alignedDEC += syncdata.deltaDEC;
            if (alignedDEC > 90.0 || alignedDEC < -90.0)
            {
                alignedRA += 12.00;
                if (alignedDEC > 0.0)
                    alignedDEC = 180.0 - alignedDEC;
                else
                    alignedDEC = -180.0 - alignedDEC;
            }
            alignedRA = range24(alignedRA);
        }

#if defined WITH_ALIGN_GEEHALEL && !defined WITH_ALIGN
        alignedRA  = ghalignedRA;
        alignedDEC = ghalignedDEC;
#endif
#if defined WITH_ALIGN_GEEHALEL && defined WITH_ALIGN
        if (AlignMethodSP.sp[0].s == ISS_ON)
        {
            alignedRA  = ghalignedRA;
            alignedDEC = ghalignedDEC;
        }
#endif

        NewRaDec(alignedRA, alignedDEC);
        lnradec.ra  = (alignedRA * 360.0) / 24.0;
        lnradec.dec = alignedDEC;
        /* uses sidereal time, not local sidereal time */
        /*ln_get_hrz_from_equ_sidereal_time(&lnradec, &lnobserver, lst, &lnaltaz);*/
        ln_get_hrz_from_equ(&lnradec, &lnobserver, juliandate, &lnaltaz);
        /* libnova measures azimuth from south towards west */
        horizvalues[0] = range360(lnaltaz.az + 180);
        horizvalues[1] = lnaltaz.alt;
        IUUpdateNumber(HorizontalCoordNP, horizvalues, (char **)horiznames, 2);
        IDSetNumber(HorizontalCoordNP, NULL);

        steppervalues[0] = currentRAEncoder;
        steppervalues[1] = currentDEEncoder;
        IUUpdateNumber(CurrentSteppersNP, steppervalues, (char **)steppernames, 2);
        IDSetNumber(CurrentSteppersNP, NULL);

        mount->GetRAMotorStatus(RAStatusLP);
        mount->GetDEMotorStatus(DEStatusLP);
        IDSetLight(RAStatusLP, NULL);
        IDSetLight(DEStatusLP, NULL);

        periods[0] = mount->GetRAPeriod();
        periods[1] = mount->GetDEPeriod();
        IUUpdateNumber(PeriodsNP, periods, (char **)periodsnames, 2);
        IDSetNumber(PeriodsNP, NULL);

        if (mount->HasAuxEncoders())
        {
            double auxencodervalues[2];
            const char *auxencodernames[] = { "AUXENCRASteps", "AUXENCDESteps" };
            auxencodervalues[0]           = mount->GetRAAuxEncoder();
            auxencodervalues[1]           = mount->GetDEAuxEncoder();
            IUUpdateNumber(AuxEncoderNP, auxencodervalues, (char **)auxencodernames, 2);
            IDSetNumber(AuxEncoderNP, NULL);
        }

        if (gotoInProgress())
        {
            if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
            {
                // Goto iteration
                gotoparams.iterative_count += 1;
                DEBUGF(INDI::Logger::DBG_SESSION,
                       "Iterative Goto (%d): RA diff = %4.2f arcsecs DE diff = %4.2f arcsecs",
                       gotoparams.iterative_count, 3600 * fabs(gotoparams.ratarget - currentRA),
                       3600 * fabs(gotoparams.detarget - currentDEC));
                if ((gotoparams.iterative_count <= GOTO_ITERATIVE_LIMIT) &&
                        (((3600 * fabs(gotoparams.ratarget - currentRA)) > RAGOTORESOLUTION) ||
                         ((3600 * fabs(gotoparams.detarget - currentDEC)) > DEGOTORESOLUTION)))
                {
                    gotoparams.racurrent        = currentRA;
                    gotoparams.decurrent        = currentDEC;
                    gotoparams.racurrentencoder = currentRAEncoder;
                    gotoparams.decurrentencoder = currentDEEncoder;
                    EncoderTarget(&gotoparams);
                    // Start iterative slewing
                    DEBUGF(INDI::Logger::DBG_SESSION,
                           "Iterative goto (%d): slew mount to RA increment = %ld, DE increment = %ld",
                           gotoparams.iterative_count, gotoparams.ratargetencoder - gotoparams.racurrentencoder,
                           gotoparams.detargetencoder - gotoparams.decurrentencoder);
                    mount->SlewTo(gotoparams.ratargetencoder - gotoparams.racurrentencoder,
                                  gotoparams.detargetencoder - gotoparams.decurrentencoder);
                }
                else
                {
                    ISwitch *sw;
                    sw = IUFindSwitch(&CoordSP, "TRACK");
                    if ((gotoparams.iterative_count > GOTO_ITERATIVE_LIMIT) &&
                            (((3600 * fabs(gotoparams.ratarget - currentRA)) > RAGOTORESOLUTION) ||
                             ((3600 * fabs(gotoparams.detarget - currentDEC)) > DEGOTORESOLUTION)))
                    {
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "Iterative Goto Limit reached (%d iterations): RA diff = %4.2f arcsecs DE diff = %4.2f "
                               "arcsecs",
                               gotoparams.iterative_count, 3600 * fabs(gotoparams.ratarget - currentRA),
                               3600 * fabs(gotoparams.detarget - currentDEC));
                    }

                    // For AstroEQ (needs an explicit :G command at the end of gotos)
                    mount->ResetMotions();

                    if ((RememberTrackState == SCOPE_TRACKING) || ((sw != NULL) && (sw->s == ISS_ON)))
                    {
                        char *name;
                        TrackState = SCOPE_TRACKING;

                        if (RememberTrackState == SCOPE_TRACKING)
                        {
                            sw   = IUFindOnSwitch(&TrackModeSP);
                            name = sw->name;
                            mount->StartRATracking(GetRATrackRate());
                            mount->StartDETracking(GetDETrackRate());
                        }
                        else
                        {
                            ISState state;
                            sw    = IUFindOnSwitch(TrackDefaultSP);
                            name  = sw->name;
                            state = ISS_ON;
                            mount->StartRATracking(GetDefaultRATrackRate());
                            mount->StartDETracking(GetDefaultDETrackRate());

#if 0
                            IUResetSwitch(TrackModeSP);
                            IUUpdateSwitch(TrackModeSP, &state, &name, 1);
                            TrackModeSP->s = IPS_BUSY;
                            IDSetSwitch(TrackModeSP, NULL);
#endif
                        }

#if 0
                        TrackModeSP->s = IPS_BUSY;
                        IDSetSwitch(TrackModeSP, NULL);
#endif
                        DEBUGF(INDI::Logger::DBG_SESSION, "Telescope slew is complete. Tracking %s...", name);
                    }
                    else
                    {
                        TrackState = SCOPE_IDLE;
                        DEBUG(INDI::Logger::DBG_SESSION, "Telescope slew is complete. Stopping...");
                    }
                    gotoparams.completed = true;
                    EqNP.s               = IPS_OK;
                }
            }
        }

#ifdef WITH_SCOPE_LIMITS
        if (horizon)
        {
            if (horizon->checkLimits(horizvalues[0], horizvalues[1], TrackState, gotoInProgress()))
                Abort();
        }
#endif

        if (TrackState == SCOPE_PARKING)
        {
            if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
            {
                SetParked(true);
            }
        }

        if (mount->HasPPEC())
        {
            if (RAPPECTrainingSP->s == IPS_BUSY)
            {
                bool intraining, inppec;
                mount->GetRAPPECStatus(&intraining, &inppec);
                if (!(intraining))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "RA PPEC Training completed.");
                    RAPPECTrainingSP->sp[0].s = ISS_ON;
                    RAPPECTrainingSP->sp[1].s = ISS_OFF;
                    RAPPECTrainingSP->s       = IPS_IDLE;
                    IDSetSwitch(RAPPECTrainingSP, NULL);
                }
            }
            if (DEPPECTrainingSP->s == IPS_BUSY)
            {
                bool intraining, inppec;
                mount->GetDEPPECStatus(&intraining, &inppec);
                if (!(intraining))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "DE PPEC Training completed.");
                    DEPPECTrainingSP->sp[0].s = ISS_ON;
                    DEPPECTrainingSP->sp[1].s = ISS_OFF;
                    DEPPECTrainingSP->s       = IPS_IDLE;
                    IDSetSwitch(DEPPECTrainingSP, NULL);
                }
            }
        }

        if (AutohomeState == AUTO_HOME_CONFIRM)
        {
            if (ah_confirm_timeout > 0)
                ah_confirm_timeout -= 1;
            if (ah_confirm_timeout == 0)
            {
                AutohomeState = AUTO_HOME_IDLE;
                DEBUG(INDI::Logger::DBG_SESSION, "Autohome confirm timeout.");
            }
        }

        if (TrackState == SCOPE_AUTOHOMING)
        {
            unsigned long indexRA = 0, indexDE = 0;

            DEBUGF(INDI::Logger::DBG_DEBUG, "Autohoming status: %d", AutohomeState);
            switch (AutohomeState)
            {
            case AUTO_HOME_IDLE:
            case AUTO_HOME_CONFIRM:
                AutohomeState = AUTO_HOME_IDLE;
                TrackState    = SCOPE_IDLE;
                DEBUG(INDI::Logger::DBG_SESSION, "Invalid status while Autohoming. Aborting");
                break;
            case AUTO_HOME_WAIT_PHASE1:
                if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 1: end");
                    DEBUG(INDI::Logger::DBG_SESSION,
                          "AutoHome phase 2: reading home position indexes for extra moves");
                    mount->GetRAIndexer();
                    mount->GetDEIndexer();
                    unsigned long raindex = mount->GetlastreadRAIndexer();
                    unsigned long deindex = mount->GetlastreadDEIndexer();
                    DEBUGF(INDI::Logger::DBG_SESSION,
                           "AutoHome phase 2: read home position indexes: RA=0x%x DE=0x%x", raindex, deindex);
                    if (raindex == 0 || raindex == 0xFFFFFF)
                        ah_bIndexChanged_RA = false;
                    else
                        ah_bIndexChanged_RA = true;
                    if (deindex == 0 || deindex == 0xFFFFFF)
                        ah_bIndexChanged_DE = false;
                    else
                        ah_bIndexChanged_DE = true;
                    if (ah_bIndexChanged_RA)
                    {
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 2: RA home index changed RA=0x%x, slewing again", raindex);
                        ah_iPosition_RA = mount->GetRAEncoder();
                        ah_iChanges     = (5 * mount->GetRAEncoderTotal()) / 360;
                        if (ah_bSlewingUp_RA)
                            ah_iPosition_RA = ah_iPosition_RA - ah_iChanges;
                        else
                            ah_iPosition_RA = ah_iPosition_RA + ah_iChanges;
                    }
                    if (ah_bIndexChanged_DE)
                    {
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 2: DE home index changed DE=0x%x, slewing again", deindex);
                        ah_iPosition_DE = mount->GetDEEncoder();
                        ah_iChanges     = (5 * mount->GetDEEncoderTotal()) / 360;
                        if (ah_bSlewingUp_DE)
                            ah_iPosition_DE = ah_iPosition_DE - ah_iChanges;
                        else
                            ah_iPosition_DE = ah_iPosition_DE + ah_iChanges;
                    }
                    if ((ah_bIndexChanged_RA) || (ah_bIndexChanged_DE))
                    {
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 2: slewing to RA=0x%x (up=%c) DE=0x%x (up=%c)", ah_iPosition_RA,
                               (ah_bSlewingUp_RA ? '1' : '0'), ah_iPosition_DE, (ah_bSlewingUp_DE ? '1' : '0'));
                        mount->AbsSlewTo(ah_iPosition_RA, ah_iPosition_DE, ah_bSlewingUp_RA, ah_bSlewingUp_DE);
                        DEBUG(INDI::Logger::DBG_SESSION,
                              "Autohome phase 2: start slewing, waiting for motors to stop");
                    }
                    else
                    {
                        DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 2: nothing to do");
                    }
                    AutohomeState = AUTO_HOME_WAIT_PHASE2;
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_DEBUG, "Autohome phase 1: Waiting for motors to stop");
                }
                break;
            case AUTO_HOME_WAIT_PHASE2:
                if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 2: end");
                    DEBUG(INDI::Logger::DBG_SESSION, "AutoHome phase 3: resetting home position indexes");
                    if (ah_bIndexChanged_RA)
                    {
                        unsigned long raindex = mount->GetlastreadRAIndexer();
                        mount->ResetRAIndexer();
                        mount->GetRAIndexer();
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 3: resetting RA home index: 0x%x (was 0x%x)",
                               mount->GetlastreadRAIndexer(), raindex);
                    }
                    if (ah_bIndexChanged_DE)
                    {
                        unsigned long deindex = mount->GetlastreadDEIndexer();
                        mount->ResetDEIndexer();
                        mount->GetDEIndexer();
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 3: resetting DE home index: 0x%x (was 0x%x)",
                               mount->GetlastreadDEIndexer(), deindex);
                    }
                    DEBUG(INDI::Logger::DBG_SESSION,
                          "AutoHome phase 3: reading home position indexes to update directions");
                    if (ah_bIndexChanged_RA)
                    {
                        mount->GetRAIndexer();
                        if (mount->GetlastreadRAIndexer() == 0)
                            ah_bSlewingUp_RA = false;
                        else
                            ah_bSlewingUp_RA = true;
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 3: reading RA home position index: RA=0x%x up=%c",
                               mount->GetlastreadRAIndexer(), (ah_bSlewingUp_RA ? '1' : '0'));
                    }
                    if (ah_bIndexChanged_DE)
                    {
                        mount->GetDEIndexer();
                        if (mount->GetlastreadDEIndexer() == 0)
                            ah_bSlewingUp_DE = false;
                        else
                            ah_bSlewingUp_DE = true;
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 3: reading DE home position index: DE=0x%x up=%c",
                               mount->GetlastreadDEIndexer(), (ah_bSlewingUp_DE ? '1' : '0'));
                    }

                    if (!ah_bSlewingUp_RA)
                    {
                        DEBUG(INDI::Logger::DBG_SESSION,
                              "AutoHome phase 3: starting RA negative slewing, waiting RA home indexer");
                        ah_waitRA = -1;
                        mount->SlewRA(-800.0);
                    }
                    if (!ah_bSlewingUp_DE)
                    {
                        DEBUG(INDI::Logger::DBG_SESSION,
                              "AutoHome phase 3: starting DE negative slewing, waiting DE home indexer");
                        ah_waitDE = -1;
                        mount->SlewDE(-800.0);
                    }
                    AutohomeState = AUTO_HOME_WAIT_PHASE3;
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_DEBUG, "Autohome phase 2: Waiting for motors to stop");
                }
                break;
            case AUTO_HOME_WAIT_PHASE3:
                if (mount->IsRARunning())
                {
                    if (ah_waitRA < 0)
                    {
                        mount->GetRAIndexer();
                        if ((indexRA = mount->GetlastreadRAIndexer()) != 0xFFFFFF)
                        {
                            ah_waitRA = 3000 / POLLMS;
                            DEBUGF(INDI::Logger::DBG_SESSION,
                                   "Autohome phase 3: detected RA Index changed, waiting %d poll periods",
                                   ah_waitRA);
                        }
                    }
                    else
                        ah_waitRA -= 1;
                    if (ah_waitRA == 0)
                    {
                        DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 3: stopping RA");
                        mount->StopRA();
                    }
                }
                if (mount->IsDERunning())
                {
                    if (ah_waitDE < 0)
                    {
                        mount->GetDEIndexer();
                        if ((indexDE = mount->GetlastreadDEIndexer()) != 0xFFFFFF)
                        {
                            ah_waitDE = 3000 / POLLMS;
                            DEBUGF(INDI::Logger::DBG_SESSION,
                                   "Autohome phase 3: detected DE Index changed, waiting %d poll periods",
                                   ah_waitDE);
                        }
                    }
                    else
                        ah_waitDE -= 1;
                    if (ah_waitDE == 0)
                    {
                        DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 3: stopping DE");
                        mount->StopDE();
                    }
                }
                if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
                {
                    if (!ah_bSlewingUp_RA)
                    {
                        mount->ResetRAIndexer();
                        mount->GetRAIndexer();
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 3: resetting RA home index: 0x%x (was 0x%x)",
                               mount->GetlastreadRAIndexer(), indexRA);
                        ah_bSlewingUp_RA = true;
                    }
                    if (!ah_bSlewingUp_DE)
                    {
                        mount->ResetDEIndexer();
                        mount->GetDEIndexer();
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 3: resetting DE home index: 0x%x (was 0x%x)",
                               mount->GetlastreadDEIndexer(), indexDE);
                        ah_bSlewingUp_DE = true;
                    }
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 3: end");
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 4: *** find the home position index ***");
                    DEBUG(INDI::Logger::DBG_SESSION,
                          "AutoHome phase 4: starting RA positive slewing, waiting RA home indexer");
                    ah_waitRA           = -1;
                    ah_bIndexChanged_RA = false;
                    mount->SlewRA(400.0);

                    DEBUG(INDI::Logger::DBG_SESSION,
                          "AutoHome phase 4: starting DE positive slewing, waiting DE home indexer");
                    ah_waitDE = -1;
                    mount->SlewDE(400.0);
                    ah_bIndexChanged_DE = false;
                    AutohomeState       = AUTO_HOME_WAIT_PHASE4;
                }
                break;
            case AUTO_HOME_WAIT_PHASE4:
                if (!ah_bIndexChanged_RA)
                {
                    mount->GetRAIndexer();
                    ah_iPosition_RA = mount->GetlastreadRAIndexer();
                    if (ah_iPosition_RA != 0)
                    {
                        ah_bIndexChanged_RA      = true;
                        ah_sHomeIndexPosition_RA = ah_iPosition_RA;
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "Autohome phase 4: detected RA Home index: 0x%x, stopping motor", ah_iPosition_RA);
                        mount->StopRA();
                    }
                }
                if (!ah_bIndexChanged_DE)
                {
                    mount->GetDEIndexer();
                    ah_iPosition_DE = mount->GetlastreadDEIndexer();
                    if (ah_iPosition_DE != 0)
                    {
                        ah_bIndexChanged_DE      = true;
                        ah_sHomeIndexPosition_DE = ah_iPosition_DE;
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "Autohome phase 4: detected DE Home index: 0x%x, stopping motor", ah_iPosition_DE);
                        mount->StopDE();
                    }
                }
                if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 4: end");
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 5: Moving back 10 deg.");
                    ah_iChanges     = (10 * mount->GetRAEncoderTotal()) / 360;
                    ah_iPosition_RA = ah_iPosition_RA - ah_iChanges;
                    ah_iChanges     = (10 * mount->GetDEEncoderTotal()) / 360;
                    ah_iPosition_DE = ah_iPosition_DE - ah_iChanges;
                    DEBUGF(INDI::Logger::DBG_SESSION,
                           "AutoHome phase 5: slewing to RA=0x%x (up=%c) DE=0x%x (up=%c)", ah_iPosition_RA, '0',
                           ah_iPosition_DE, '0');
                    mount->AbsSlewTo(ah_iPosition_RA, ah_iPosition_DE, false, false);
                    AutohomeState = AUTO_HOME_WAIT_PHASE5;
                }
                break;
            case AUTO_HOME_WAIT_PHASE5:
                if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 5: end");
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 6: Goto Home Position");
                    DEBUGF(INDI::Logger::DBG_SESSION,
                           "AutoHome phase 6: slewing to RA=0x%x (up=%c) DE=0x%x (up=%c)", ah_sHomeIndexPosition_RA,
                           '1', ah_sHomeIndexPosition_DE, '1');
                    mount->AbsSlewTo(ah_sHomeIndexPosition_RA, ah_sHomeIndexPosition_DE, true, true);
                    AutohomeState = AUTO_HOME_WAIT_PHASE6;
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_DEBUG, "Autohome phase 5: Waiting for motors to stop");
                }
                break;
            case AUTO_HOME_WAIT_PHASE6:
                if (!(mount->IsRARunning()) && !(mount->IsDERunning()))
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome phase 6: end");
                    DEBUGF(INDI::Logger::DBG_SESSION, "AutoHome phase 6: Mount at RA=0x%x DE=0x%x",
                           mount->GetRAEncoder(), mount->GetDEEncoder());
                    DEBUGF(INDI::Logger::DBG_SESSION,
                           "Autohome: Mount at Home Position, setting encoders RA=0x%x DE=0X%x",
                           mount->GetRAEncoderHome(), mount->GetDEEncoderHome());
                    mount->SetRAAxisPosition(mount->GetRAEncoderHome());
                    mount->SetDEAxisPosition(mount->GetDEEncoderHome());
                    TrackState    = SCOPE_IDLE;
                    AutohomeState = AUTO_HOME_IDLE;
                    AutoHomeSP->s = IPS_IDLE;
                    IUResetSwitch(AutoHomeSP);
                    IDSetSwitch(AutoHomeSP, NULL);
                    DEBUG(INDI::Logger::DBG_SESSION, "Autohome: end");
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_DEBUG, "Autohome phase 6: Waiting for motors to stop");
                }
                break;
            default:
                DEBUGF(INDI::Logger::DBG_WARNING, "Unknown Autohome status %d: aborting", AutohomeState);
                Abort();
                break;
            }
        }
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

    return true;
}

void EQMod::EncodersToRADec(unsigned long rastep, unsigned long destep, double lst, double *ra, double *de, double *ha)
{
    double RACurrent = 0.0, DECurrent = 0.0, HACurrent = 0.0;
    HACurrent = EncoderToHours(rastep, zeroRAEncoder, totalRAEncoder, Hemisphere);
    RACurrent = HACurrent + lst;
    DECurrent = EncoderToDegrees(destep, zeroDEEncoder, totalDEEncoder, Hemisphere);
    //IDLog("EncodersToRADec: destep=%6X zeroDEncoder=%6X totalDEEncoder=%6x DECurrent=%f\n", destep, zeroDEEncoder , totalDEEncoder, DECurrent);
    if (Hemisphere == NORTH)
    {
        if ((DECurrent > 90.0) && (DECurrent <= 270.0))
        {
            RACurrent = RACurrent - 12.0;
            //currentPierSide = EAST;
            setPierSide(PIER_EAST);
        }
        else
            setPierSide(PIER_WEST);
        //currentPierSide = WEST;
    }
    else if ((DECurrent <= 90.0) || (DECurrent > 270.0))
    {
        RACurrent = RACurrent + 12.0;
        //currentPierSide = EAST;
        setPierSide(PIER_EAST);
    }
    else
        setPierSide(PIER_WEST);
    //currentPierSide = WEST;
    HACurrent = rangeHA(HACurrent);
    RACurrent = range24(RACurrent);
    DECurrent = rangeDec(DECurrent);
    *ra       = RACurrent;
    *de       = DECurrent;
    if (ha)
        *ha = HACurrent;
}

double EQMod::EncoderToHours(unsigned long step, unsigned long initstep, unsigned long totalstep, enum Hemisphere h)
{
    double result = 0.0;
    if (step > initstep)
    {
        result = ((double)(step - initstep) / totalstep) * 24.0;
        result = 24.0 - result;
    }
    else
    {
        result = ((double)(initstep - step) / totalstep) * 24.0;
    }

    if (h == NORTH)
        result = range24(result + 6.0);
    else
        result = range24((24 - result) + 6.0);
    return result;
}

double EQMod::EncoderToDegrees(unsigned long step, unsigned long initstep, unsigned long totalstep, enum Hemisphere h)
{
    double result = 0.0;
    if (step > initstep)
    {
        result = ((double)(step - initstep) / totalstep) * 360.0;
    }
    else
    {
        result = ((double)(initstep - step) / totalstep) * 360.0;
        result = 360.0 - result;
    }
    //IDLog("EncodersToDegrees: step=%6X initstep=%6x result=%f hemisphere %s \n", step, initstep, result, (h==NORTH?"North":"South"));
    if (h == NORTH)
        result = range360(result);
    else
        result = range360(360.0 - result);
    //IDLog("EncodersToDegrees: returning result=%f\n", result);

    return result;
}

double EQMod::EncoderFromHour(double hour, unsigned long initstep, unsigned long totalstep, enum Hemisphere h)
{
    double shifthour = 0.0;
    shifthour        = range24(hour - 6);
    if (h == NORTH)
        if (shifthour < 12.0)
            return (initstep - ((shifthour / 24.0) * totalstep));
        else
            return (initstep + (((24.0 - shifthour) / 24.0) * totalstep));
    else if (shifthour < 12.0)
        return (initstep + ((shifthour / 24.0) * totalstep));
    else
        return (initstep - (((24.0 - shifthour) / 24.0) * totalstep));
}

double EQMod::EncoderFromRA(double ratarget, double detarget, double lst, unsigned long initstep,
                            unsigned long totalstep, enum Hemisphere h)
{
    double ha = 0.0;
    ha        = ratarget - lst;

    // used only in simulation??
    if (h == NORTH)
        if ((detarget > 90.0) && (detarget <= 270.0))
            ha = ha - 12.0;
    if (h == SOUTH)
        if ((detarget > 90.0) && (detarget <= 270.0))
            ha = ha + 12.0;

    ha = range24(ha);
    return EncoderFromHour(ha, initstep, totalstep, h);
}

double EQMod::EncoderFromDegree(double degree, TelescopePierSide p, unsigned long initstep, unsigned long totalstep,
                                enum Hemisphere h)
{
    double target = 0.0;
    target        = degree;
    if (h == SOUTH)
        target = 360.0 - target;
    if ((target > 180.0) && (p == PIER_EAST))
        return (initstep - (((360.0 - target) / 360.0) * totalstep));
    else
        return (initstep + ((target / 360.0) * totalstep));
}
double EQMod::EncoderFromDec(double detarget, TelescopePierSide p, unsigned long initstep, unsigned long totalstep,
                             enum Hemisphere h)
{
    double target = 0.0;
    target        = detarget;
    if (p == PIER_WEST)
        target = 180.0 - target;
    return EncoderFromDegree(target, p, initstep, totalstep, h);
}

void EQMod::SetSouthernHemisphere(bool southern)
{
    const char *hemispherenames[] = { "NORTH", "SOUTH" };
    ISState hemispherevalues[2];
    DEBUGF(INDI::Logger::DBG_DEBUG, "Set southern %s", (southern ? "true" : "false"));
    if (southern)
        Hemisphere = SOUTH;
    else
        Hemisphere = NORTH;
    RAInverted = (Hemisphere == SOUTH);
    DEInverted = ((Hemisphere == SOUTH) ^ (getPierSide() == PIER_WEST));
    if (Hemisphere == NORTH)
    {
        hemispherevalues[0] = ISS_ON;
        hemispherevalues[1] = ISS_OFF;
        IUUpdateSwitch(HemisphereSP, hemispherevalues, (char **)hemispherenames, 2);
    }
    else
    {
        hemispherevalues[0] = ISS_OFF;
        hemispherevalues[1] = ISS_ON;
        IUUpdateSwitch(HemisphereSP, hemispherevalues, (char **)hemispherenames, 2);
    }
    HemisphereSP->s = IPS_IDLE;
    IDSetSwitch(HemisphereSP, NULL);
}

void EQMod::EncoderTarget(GotoParams *g)
{
    double r, d;
    double ha = 0.0, targetra = 0.0;
    double juliandate;
    double lst;
    TelescopePierSide targetpier;
    unsigned long targetraencoder = 0, targetdecencoder = 0;
    bool outsidelimits = false;
    r                  = g->ratarget;
    d                  = g->detarget;

    juliandate = getJulianDate();
    lst        = getLst(juliandate, getLongitude());

    ha = rangeHA(r - lst);
    if (ha < 0.0)
    { // target EAST
        if (g->forcecwup)
        {
            if (Hemisphere == NORTH)
                targetpier = PIER_EAST;
            else
                targetpier = PIER_WEST;
            targetra = r;
        }
        else
        {
            if (Hemisphere == NORTH)
                targetpier = PIER_WEST;
            else
                targetpier = PIER_EAST;
            targetra = range24(r - 12.0);
        }
    }
    else
    {
        if (g->forcecwup)
        {
            if (Hemisphere == NORTH)
                targetpier = PIER_WEST;
            else
                targetpier = PIER_EAST;
            targetra = range24(r - 12.0);
        }
        else
        {
            if (Hemisphere == NORTH)
                targetpier = PIER_EAST;
            else
                targetpier = PIER_WEST;
            targetra = r;
        }
    }

    targetraencoder  = EncoderFromRA(targetra, 0.0, lst, zeroRAEncoder, totalRAEncoder, Hemisphere);
    targetdecencoder = EncoderFromDec(d, targetpier, zeroDEEncoder, totalDEEncoder, Hemisphere);

    if ((g->forcecwup) && (g->checklimits))
    {
        if (Hemisphere == NORTH)
        {
            if ((targetraencoder < g->limiteast) || (targetraencoder > g->limitwest))
                outsidelimits = true;
        }
        else
        {
            if ((targetraencoder > g->limiteast) || (targetraencoder < g->limitwest))
                outsidelimits = true;
        }
        if (outsidelimits)
        {
            DEBUG(INDI::Logger::DBG_WARNING, "Goto: RA Limits prevent Counterweights-up slew.");
            if (ha < 0.0)
            { // target EAST
                if (Hemisphere == NORTH)
                    targetpier = PIER_WEST;
                else
                    targetpier = PIER_EAST;
                targetra = range24(r - 12.0);
            }
            else
            {
                if (Hemisphere == NORTH)
                    targetpier = PIER_EAST;
                else
                    targetpier = PIER_WEST;
                targetra = r;
            }
            targetraencoder  = EncoderFromRA(targetra, 0.0, lst, zeroRAEncoder, totalRAEncoder, Hemisphere);
            targetdecencoder = EncoderFromDec(d, targetpier, zeroDEEncoder, totalDEEncoder, Hemisphere);
        }
    }
    g->outsidelimits   = outsidelimits;
    g->ratargetencoder = targetraencoder;
    g->detargetencoder = targetdecencoder;
}

double EQMod::GetRATrackRate()
{
    double rate = 0.0;
    ISwitch *sw;
    sw = IUFindOnSwitch(&TrackModeSP);
    if (!sw)
        return 0.0;
    if (!strcmp(sw->name, "TRACK_SIDEREAL"))
    {
        rate = TRACKRATE_SIDEREAL;
    }
    else if (!strcmp(sw->name, "TRACK_LUNAR"))
    {
        rate = TRACKRATE_LUNAR;
    }
    else if (!strcmp(sw->name, "TRACK_SOLAR"))
    {
        rate = TRACKRATE_SOLAR;
    }
    else if (!strcmp(sw->name, "TRACK_CUSTOM"))
    {
        rate = IUFindNumber(&TrackRateNP, "TRACK_RATE_RA")->value;
    }
    else
        return 0.0;
    if (RAInverted)
        rate = -rate;
    return rate;
}

double EQMod::GetDETrackRate()
{
    double rate = 0.0;
    ISwitch *sw;
    sw = IUFindOnSwitch(&TrackModeSP);
    if (!sw)
        return 0.0;
    if (!strcmp(sw->name, "TRACK_SIDEREAL"))
    {
        rate = 0.0;
    }
    else if (!strcmp(sw->name, "TRACK_LUNAR"))
    {
        rate = 0.0;
    }
    else if (!strcmp(sw->name, "TRACK_SOLAR"))
    {
        rate = 0.0;
    }
    else if (!strcmp(sw->name, "TRACK_CUSTOM"))
    {
        rate = IUFindNumber(&TrackRateNP, "TRACK_RATE_DE")->value;
    }
    else
        return 0.0;
    if (DEInverted)
        rate = -rate;
    return rate;
}

double EQMod::GetDefaultRATrackRate()
{
    double rate = 0.0;
    ISwitch *sw;
    sw = IUFindOnSwitch(TrackDefaultSP);
    if (!sw)
        return 0.0;
    if (!strcmp(sw->name, "TRACK_SIDEREAL"))
    {
        rate = TRACKRATE_SIDEREAL;
    }
    else if (!strcmp(sw->name, "TRACK_LUNAR"))
    {
        rate = TRACKRATE_LUNAR;
    }
    else if (!strcmp(sw->name, "TRACK_SOLAR"))
    {
        rate = TRACKRATE_SOLAR;
    }
    else if (!strcmp(sw->name, "TRACK_CUSTOM"))
    {
        rate = IUFindNumber(&TrackRateNP, "TRACK_RATE_RA")->value;
    }
    else
        return 0.0;
    if (RAInverted)
        rate = -rate;
    return rate;
}

double EQMod::GetDefaultDETrackRate()
{
    double rate = 0.0;
    ISwitch *sw;
    sw = IUFindOnSwitch(TrackDefaultSP);
    if (!sw)
        return 0.0;
    if (!strcmp(sw->name, "TRACK_SIDEREAL"))
    {
        rate = 0.0;
    }
    else if (!strcmp(sw->name, "TRACK_LUNAR"))
    {
        rate = 0.0;
    }
    else if (!strcmp(sw->name, "TRACK_SOLAR"))
    {
        rate = 0.0;
    }
    else if (!strcmp(sw->name, "TRACK_CUSTOM"))
    {
        rate = IUFindNumber(&TrackRateNP, "TRACK_RATE_DE")->value;
    }
    else
        return 0.0;
    if (DEInverted)
        rate = -rate;
    return rate;
}

bool EQMod::gotoInProgress()
{
    return (!gotoparams.completed);
}

bool EQMod::Goto(double r, double d)
{
    double juliandate;
    double lst;
#ifdef WITH_SCOPE_LIMITS
    struct ln_equ_posn gotoradec;
    struct ln_hrz_posn gotoaltaz;
    double gotoaz;
    double gotoalt;
#endif

    if ((TrackState == SCOPE_SLEWING) || (TrackState == SCOPE_PARKING) || (TrackState == SCOPE_PARKED))
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Can not perform goto while goto/park in progress, or scope parked.");
        EqNP.s = IPS_IDLE;
        IDSetNumber(&EqNP, NULL);
        return true;
    }

    juliandate = getJulianDate();
    lst        = getLst(juliandate, getLongitude());

#ifdef WITH_SCOPE_LIMITS
    gotoradec.ra  = (r * 360.0) / 24.0;
    gotoradec.dec = d;
    /* uses sidereal time, not local sidereal time */
    /*ln_get_hrz_from_equ_sidereal_time(&lnradec, &lnobserver, lst, &lnaltaz);*/
    ln_get_hrz_from_equ(&gotoradec, &lnobserver, juliandate, &gotoaltaz);
    /* libnova measures azimuth from south towards west */
    gotoaz  = range360(gotoaltaz.az + 180);
    gotoalt = gotoaltaz.alt;
    if (horizon)
    {
        if (!horizon->inGotoLimits(gotoaz, gotoalt))
        {
            DEBUG(INDI::Logger::DBG_WARNING, "Goto outside Horizon Limits.");
            EqNP.s = IPS_IDLE;
            IDSetNumber(&EqNP, NULL);
            return true;
        }
    }
#endif

    DEBUGF(INDI::Logger::DBG_SESSION, "Starting Goto RA=%g DE=%g (current RA=%g DE=%g)", r, d, currentRA, currentDEC);
    targetRA  = r;
    targetDEC = d;
    char RAStr[64], DecStr[64];

    // Compute encoder targets and check RA limits if forced
    bzero(&gotoparams, sizeof(gotoparams));
    gotoparams.ratarget  = r;
    gotoparams.detarget  = d;
    gotoparams.racurrent = currentRA;
    gotoparams.decurrent = currentDEC;
    bool aligned         = false;
#ifdef WITH_ALIGN_GEEHALEL
    double ghratarget = r, ghdetarget = d;
    aligned = true;
    if (align)
    {
        align->AlignGoto(syncdata, juliandate, &lnobserver, &ghratarget, &ghdetarget);
        DEBUGF(INDI::Logger::DBG_SESSION, "Aligned Eqmod Goto RA=%g DE=%g (target RA=%g DE=%g)", ghratarget, ghdetarget,
               r, d);
    }
    else
    {
        if (syncdata.lst != 0.0)
        {
            ghratarget = gotoparams.ratarget - syncdata.deltaRA;
            ghdetarget = gotoparams.detarget - syncdata.deltaDEC;
            DEBUGF(INDI::Logger::DBG_SESSION, "Failed Eqmod Goto RA=%g DE=%g (target RA=%g DE=%g)", ghratarget,
                   ghdetarget, r, d);
        }
    }
#endif
#ifdef WITH_ALIGN
    TelescopeDirectionVector TDV;
    aligned = true;
    if ((GetAlignmentDatabase().size() < 2) || (!TransformCelestialToTelescope(r, d, 0.0, TDV)))
    {
        //if (!TransformCelestialToTelescope(r, d, 0.0, TDV)) {
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
               "Failed TransformCelestialToTelescope:  RA=%lf DE=%lf, Goto RA=%lf DE=%lf", r, d, gotoparams.ratarget,
               gotoparams.detarget);
        if (syncdata.lst != 0.0)
        {
            gotoparams.ratarget -= syncdata.deltaRA;
            gotoparams.detarget -= syncdata.deltaDEC;
        }
    }
    else
    {
        struct ln_equ_posn RaDec;
        LocalHourAngleDeclinationFromTelescopeDirectionVector(TDV, RaDec);
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
               "TransformCelestialToTelescope: RA=%lf DE=%lf, TDV (x :%lf, y: %lf, z: %lf), local hour RA %lf DEC %lf",
               r, d, TDV.x, TDV.y, TDV.z, RaDec.ra, RaDec.dec);
        RaDec.ra = (RaDec.ra * 24.0) / 360.0;
        RaDec.ra = range24(lst - RaDec.ra);

        gotoparams.ratarget = RaDec.ra;
        gotoparams.detarget = RaDec.dec;
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
               "TransformCelestialToTelescope: RA=%lf DE=%lf, Goto RA=%lf DE=%lf", r, d, gotoparams.ratarget,
               gotoparams.detarget);
    }
#endif

    if (!aligned && (syncdata.lst != 0.0))
    {
        gotoparams.ratarget -= syncdata.deltaRA;
        gotoparams.detarget -= syncdata.deltaDEC;
    }

#if defined WITH_ALIGN_GEEHALEL && !defined WITH_ALIGN
    if (aligned)
    {
        gotoparams.ratarget = ghratarget;
        gotoparams.detarget = ghdetarget;
    }
#endif
#if defined WITH_ALIGN_GEEHALEL && defined WITH_ALIGN
    if (aligned && (AlignMethodSP.sp[0].s == ISS_ON))
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Setting Eqmod Goto RA=%g DE=%g (target RA=%g DE=%g)", ghratarget, ghdetarget,
               r, d);
        gotoparams.ratarget = ghratarget;
        gotoparams.detarget = ghdetarget;
    }
#endif

    gotoparams.racurrentencoder = currentRAEncoder;
    gotoparams.decurrentencoder = currentDEEncoder;
    gotoparams.completed        = false;
    gotoparams.checklimits      = true;
    gotoparams.forcecwup        = false;
    gotoparams.outsidelimits    = false;
    gotoparams.limiteast        = zeroRAEncoder - (totalRAEncoder / 4) - (totalRAEncoder / 24); // 13h
    gotoparams.limitwest        = zeroRAEncoder + (totalRAEncoder / 4) + (totalRAEncoder / 24); // 23h
    EncoderTarget(&gotoparams);
    try
    {
        // stop motor
        mount->StopRA();
        mount->StopDE();
        // Start slewing
        DEBUGF(INDI::Logger::DBG_SESSION, "Slewing mount: RA increment = %ld, DE increment = %ld",
               gotoparams.ratargetencoder - gotoparams.racurrentencoder,
               gotoparams.detargetencoder - gotoparams.decurrentencoder);
        mount->SlewTo(gotoparams.ratargetencoder - gotoparams.racurrentencoder,
                      gotoparams.detargetencoder - gotoparams.decurrentencoder);
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

    fs_sexa(RAStr, targetRA, 2, 3600);
    fs_sexa(DecStr, targetDEC, 2, 3600);

    // This is already set before Goto in INDI::Telescope
    //RememberTrackState = TrackState;

    TrackState         = SCOPE_SLEWING;

    //EqREqNP.s = IPS_BUSY;
    EqNP.s = IPS_BUSY;

#if 0
    // 2017-08-01 Jasem: We should set TrackState to IPS_IDLE instead here?
    TrackModeSP->s = IPS_IDLE;
    IDSetSwitch(TrackModeSP, NULL);
#endif

    DEBUGF(INDI::Logger::DBG_SESSION, "Slewing to RA: %s - DEC: %s", RAStr, DecStr);
    return true;
}

bool EQMod::Park()
{
    if (!isParked())
    {
        if (TrackState == SCOPE_SLEWING)
        {
            DEBUG(INDI::Logger::DBG_SESSION, "Can not park while slewing...");
            ParkSP.s = IPS_ALERT;
            IDSetSwitch(&ParkSP, NULL);
            return false;
        }

        try
        {
            // stop motor
            mount->StopRA();
            mount->StopDE();
            currentRAEncoder = mount->GetRAEncoder();
            currentDEEncoder = mount->GetDEEncoder();
            parkRAEncoder    = GetAxis1Park();
            parkDEEncoder    = GetAxis2Park();
            // Start slewing
            DEBUGF(INDI::Logger::DBG_SESSION, "Parking mount: RA increment = %ld, DE increment = %ld",
                   parkRAEncoder - currentRAEncoder, parkDEEncoder - currentDEEncoder);
            mount->SlewTo(parkRAEncoder - currentRAEncoder, parkDEEncoder - currentDEEncoder);
        }
        catch (EQModError e)
        {
            return (e.DefaultHandleException(this));
        }
        //TrackModeSP->s = IPS_IDLE;
        //IDSetSwitch(TrackModeSP, NULL);
        TrackState = SCOPE_PARKING;
        ParkSP.s   = IPS_BUSY;
        IDSetSwitch(&ParkSP, NULL);
        DEBUG(INDI::Logger::DBG_SESSION, "Telescope park in progress...");

        return true;
    }

    return false;
}

bool EQMod::UnPark()
{
    SetParked(false);
    return true;
}

bool EQMod::Sync(double ra, double dec)
{
    double juliandate;
    double lst;
    SyncData tmpsyncdata;
    double ha, targetra;
    TelescopePierSide targetpier;
    double telescopeHA;

    // get current mount position asap
    tmpsyncdata.telescopeRAEncoder  = mount->GetRAEncoder();
    tmpsyncdata.telescopeDECEncoder = mount->GetDEEncoder();

    juliandate = getJulianDate();
    lst        = getLst(juliandate, getLongitude());

    if (TrackState != SCOPE_TRACKING)
    {
        EqNP.s = IPS_ALERT;
        IDSetNumber(&EqNP, NULL);
        DEBUG(INDI::Logger::DBG_WARNING, "Syncs are allowed only when Tracking");
        return false;
    }
    /* remember the two last syncs to compute Polar alignment */

    tmpsyncdata.lst       = lst;
    tmpsyncdata.jd        = juliandate;
    tmpsyncdata.targetRA  = ra;
    tmpsyncdata.targetDEC = dec;

    ha = rangeHA(ra - lst);
    if (ha < 0.0)
    { // target EAST
        if (Hemisphere == NORTH)
            targetpier = PIER_WEST;
        else
            targetpier = PIER_EAST;
        targetra = range24(ra - 12.0);
    }
    else
    {
        if (Hemisphere == NORTH)
            targetpier = PIER_EAST;
        else
            targetpier = PIER_WEST;
        targetra = ra;
    }
    tmpsyncdata.targetRAEncoder  = EncoderFromRA(targetra, 0.0, lst, zeroRAEncoder, totalRAEncoder, Hemisphere);
    tmpsyncdata.targetDECEncoder = EncoderFromDec(dec, targetpier, zeroDEEncoder, totalDEEncoder, Hemisphere);

    try
    {
        EncodersToRADec(tmpsyncdata.telescopeRAEncoder, tmpsyncdata.telescopeDECEncoder, lst, &tmpsyncdata.telescopeRA,
                        &tmpsyncdata.telescopeDEC, &telescopeHA);
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

    tmpsyncdata.deltaRA         = tmpsyncdata.targetRA - tmpsyncdata.telescopeRA;
    tmpsyncdata.deltaDEC        = tmpsyncdata.targetDEC - tmpsyncdata.telescopeDEC;
    tmpsyncdata.deltaRAEncoder  = tmpsyncdata.targetRAEncoder - tmpsyncdata.telescopeRAEncoder;
    tmpsyncdata.deltaDECEncoder = tmpsyncdata.targetDECEncoder - tmpsyncdata.telescopeDECEncoder;
#ifdef WITH_ALIGN_GEEHALEL
    if (align && !isStandardSync())
    {
        align->AlignSync(syncdata, tmpsyncdata);
        //return true;
    }
#endif
#ifdef WITH_ALIGN
    if (!isStandardSync())
    {
        AlignmentDatabaseEntry NewEntry;
        struct ln_equ_posn RaDec;
        RaDec.ra  = ((lst - tmpsyncdata.telescopeRA) * 360.0) / 24.0;
        RaDec.dec = tmpsyncdata.telescopeDEC;
        //NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
        NewEntry.ObservationJulianDate = juliandate;
        NewEntry.RightAscension        = ra;
        NewEntry.Declination           = dec;
        NewEntry.TelescopeDirection    = TelescopeDirectionVectorFromLocalHourAngleDeclination(RaDec);
        NewEntry.PrivateDataSize       = 0;
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "New sync point Date %lf RA %lf DEC %lf TDV(x %lf y %lf z %lf)",
               NewEntry.ObservationJulianDate, NewEntry.RightAscension, NewEntry.Declination,
               NewEntry.TelescopeDirection.x, NewEntry.TelescopeDirection.y, NewEntry.TelescopeDirection.z);
        if (!CheckForDuplicateSyncPoint(NewEntry))
        {
            GetAlignmentDatabase().push_back(NewEntry);

            // Tell the client about size change
            UpdateSize();

            // Tell the math plugin to reinitialise
            Initialise(this);

            //if (GetAlignmentDatabase().size() >= 2)  return true;
        }
        //if (GetAlignmentDatabase().size() >= 2) return false;
    }
#endif
#if defined WITH_ALIGN_GEEHALEL || defined WITH_ALIGN
    if (isStandardSync())
#endif
    {
#ifdef WITH_ALIGN_GEEHALEL
        if (align && isStandardSync())
            align->AlignStandardSync(syncdata, &tmpsyncdata, &lnobserver);
#endif
        syncdata2 = syncdata;
        syncdata  = tmpsyncdata;

        IUFindNumber(StandardSyncNP, "STANDARDSYNC_RA")->value = syncdata.deltaRA;
        IUFindNumber(StandardSyncNP, "STANDARDSYNC_DE")->value = syncdata.deltaDEC;
        IDSetNumber(StandardSyncNP, NULL);
        IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_JD")->value           = juliandate;
        IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_SYNCTIME")->value     = lst;
        IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_CELESTIAL_RA")->value = syncdata.targetRA;
        ;
        IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_CELESTIAL_DE")->value = syncdata.targetDEC;
        ;
        IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_TELESCOPE_RA")->value = syncdata.telescopeRA;
        ;
        IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_TELESCOPE_DE")->value = syncdata.telescopeDEC;
        ;
        IDSetNumber(StandardSyncPointNP, NULL);

        DEBUGF(INDI::Logger::DBG_SESSION, "Mount Synced (deltaRA = %.6f deltaDEC = %.6f)", syncdata.deltaRA,
               syncdata.deltaDEC);
        //IDLog("Mount Synced (deltaRA = %.6f deltaDEC = %.6f)\n", syncdata.deltaRA, syncdata.deltaDEC);
        if (syncdata2.lst != 0.0)
        {
            computePolarAlign(syncdata2, syncdata, getLatitude(), &tpa_alt, &tpa_az);
            IUFindNumber(SyncPolarAlignNP, "SYNCPOLARALIGN_ALT")->value = tpa_alt;
            IUFindNumber(SyncPolarAlignNP, "SYNCPOLARALIGN_AZ")->value  = tpa_az;
            IDSetNumber(SyncPolarAlignNP, NULL);
            IDLog("computePolarAlign: Telescope Polar Axis: alt = %g, az = %g\n", tpa_alt, tpa_az);
        }
    }
    return true;
}

IPState EQMod::GuideNorth(float ms)
{
    double rateshift = 0.0;
    rateshift        = TRACKRATE_SIDEREAL * IUFindNumber(GuideRateNP, "GUIDE_RATE_NS")->value;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Timed guide North %d ms at rate %g", (int)(ms), rateshift);
    if (DEInverted)
        rateshift = -rateshift;
    try
    {
        if (ms > 0.0)
        {
            if (mount->HasPPEC())
            {
                restartguideDEPPEC = false;
                if (DEPPECSP->s == IPS_BUSY)
                {
                    restartguideDEPPEC = true;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning DEC PPEC off while guiding.");
                    mount->TurnDEPPEC(false);
                }
            }
            mount->StartDETracking(GetDETrackRate() + rateshift);
            GuideTimerNS = IEAddTimer((int)(ms), (IE_TCF *)timedguideNSCallback, this);
        }
    }
    catch (EQModError e)
    {
        e.DefaultHandleException(this);
        return IPS_ALERT;
    }

    return IPS_BUSY;
}

IPState EQMod::GuideSouth(float ms)
{
    double rateshift = 0.0;
    rateshift        = TRACKRATE_SIDEREAL * IUFindNumber(GuideRateNP, "GUIDE_RATE_NS")->value;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Timed guide South %d ms at rate %g", (int)(ms), rateshift);
    if (DEInverted)
        rateshift = -rateshift;
    try
    {
        if (ms > 0.0)
        {
            if (mount->HasPPEC())
            {
                restartguideDEPPEC = false;
                if (DEPPECSP->s == IPS_BUSY)
                {
                    restartguideDEPPEC = true;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning DEC PPEC off while guiding.");
                    mount->TurnDEPPEC(false);
                }
            }
            mount->StartDETracking(GetDETrackRate() - rateshift);
            GuideTimerNS = IEAddTimer((int)(ms), (IE_TCF *)timedguideNSCallback, this);
        }
    }
    catch (EQModError e)
    {
        e.DefaultHandleException(this);
        return IPS_ALERT;
    }
    return IPS_BUSY;
}

IPState EQMod::GuideEast(float ms)
{
    double rateshift = 0.0;
    rateshift        = TRACKRATE_SIDEREAL * IUFindNumber(GuideRateNP, "GUIDE_RATE_WE")->value;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Timed guide East %d ms at rate %g", (int)(ms), rateshift);
    if (RAInverted)
        rateshift = -rateshift;
    try
    {
        if (ms > 0.0)
        {
            if (mount->HasPPEC())
            {
                restartguideRAPPEC = false;
                if (RAPPECSP->s == IPS_BUSY)
                {
                    restartguideRAPPEC = true;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning RA PPEC off while guiding.");
                    mount->TurnRAPPEC(false);
                }
            }
            mount->StartRATracking(GetRATrackRate() - rateshift);
            GuideTimerWE = IEAddTimer((int)(ms), (IE_TCF *)timedguideWECallback, this);
        }
    }
    catch (EQModError e)
    {
        e.DefaultHandleException(this);
        return IPS_ALERT;
    }

    return IPS_BUSY;
}

IPState EQMod::GuideWest(float ms)
{
    double rateshift = 0.0;
    rateshift        = TRACKRATE_SIDEREAL * IUFindNumber(GuideRateNP, "GUIDE_RATE_WE")->value;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Timed guide West %d ms at rate %g", (int)(ms), rateshift);
    if (RAInverted)
        rateshift = -rateshift;
    try
    {
        if (ms > 0.0)
        {
            if (mount->HasPPEC())
            {
                restartguideRAPPEC = false;
                if (RAPPECSP->s == IPS_BUSY)
                {
                    restartguideRAPPEC = true;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning RA PPEC off while guiding.");
                    mount->TurnRAPPEC(false);
                }
            }
            mount->StartRATracking(GetRATrackRate() + rateshift);
            GuideTimerWE = IEAddTimer((int)(ms), (IE_TCF *)timedguideWECallback, this);
        }
    }
    catch (EQModError e)
    {
        e.DefaultHandleException(this);
        return IPS_ALERT;
    }

    return IPS_BUSY;
}

bool EQMod::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    bool compose = true;
    //  first check if it's for our device
    if (strcmp(dev, getDeviceName()) == 0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here

        if (strcmp(name, "SLEWSPEEDS") == 0)
        {
            /* TODO: don't change speed in gotos gotoparams.inprogress... */
            if (TrackState != SCOPE_TRACKING)
            {
                try
                {
                    for (int i = 0; i < n; i++)
                    {
                        if (strcmp(names[i], "RASLEW") == 0)
                            mount->SetRARate(values[i]);
                        else if (strcmp(names[i], "DESLEW") == 0)
                            mount->SetDERate(values[i]);
                    }
                }
                catch (EQModError e)
                {
                    return (e.DefaultHandleException(this));
                }
            }
            IUUpdateNumber(SlewSpeedsNP, values, names, n);
            SlewSpeedsNP->s = IPS_OK;
            IDSetNumber(SlewSpeedsNP, NULL);
            DEBUGF(INDI::Logger::DBG_SESSION, "Setting Slew rates - RA=%.2fx DE=%.2fx",
                   IUFindNumber(SlewSpeedsNP, "RASLEW")->value, IUFindNumber(SlewSpeedsNP, "DESLEW")->value);
            return true;
        }

        // Guider interface
        if (!strcmp(name, GuideNSNP.name) || !strcmp(name, GuideWENP.name))
        {
            // Unless we're in track mode, we don't obey guide commands.
            if (TrackState != SCOPE_TRACKING)
            {
                GuideNSNP.s = IPS_IDLE;
                IDSetNumber(&GuideNSNP, NULL);
                GuideWENP.s = IPS_IDLE;
                IDSetNumber(&GuideWENP, NULL);
                DEBUG(INDI::Logger::DBG_WARNING, "Can not guide if not tracking.");
                return true;
            }

            processGuiderProperties(name, values, names, n);

            return true;
        }
        if (strcmp(name, GuideRateNP->name) == 0)
        {
            IUUpdateNumber(GuideRateNP, values, names, n);
            GuideRateNP->s = IPS_OK;
            IDSetNumber(GuideRateNP, NULL);
            DEBUGF(INDI::Logger::DBG_SESSION, "Setting Custom Tracking Rates - RA=%1.1f arcsec/s DE=%1.1f arcsec/s",
                   IUFindNumber(GuideRateNP, "GUIDE_RATE_WE")->value,
                   IUFindNumber(GuideRateNP, "GUIDE_RATE_NS")->value);
            return true;
        }

        if (strcmp(name, "BACKLASH") == 0)
        {
            IUUpdateNumber(BacklashNP, values, names, n);
            BacklashNP->s = IPS_OK;
            IDSetNumber(BacklashNP, NULL);
            mount->SetBacklashRA((unsigned long)(IUFindNumber(BacklashNP, "BACKLASHRA")->value));
            mount->SetBacklashDE((unsigned long)(IUFindNumber(BacklashNP, "BACKLASHDE")->value));
            DEBUGF(INDI::Logger::DBG_SESSION, "Setting Backlash compensation - RA=%.0f microsteps DE=%.0f microsteps",
                   IUFindNumber(BacklashNP, "BACKLASHRA")->value, IUFindNumber(BacklashNP, "BACKLASHDE")->value);
            return true;
        }

        if (strcmp(name, "STANDARDSYNCPOINT") == 0)
        {
            syncdata2 = syncdata;
            bzero(&syncdata, sizeof(syncdata));
            IUUpdateNumber(StandardSyncPointNP, values, names, n);
            StandardSyncPointNP->s = IPS_OK;

            syncdata.jd           = IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_JD")->value;
            syncdata.lst          = IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_SYNCTIME")->value;
            syncdata.targetRA     = IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_CELESTIAL_RA")->value;
            syncdata.targetDEC    = IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_CELESTIAL_DE")->value;
            syncdata.telescopeRA  = IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_TELESCOPE_RA")->value;
            syncdata.telescopeDEC = IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_TELESCOPE_DE")->value;
            syncdata.deltaRA      = syncdata.targetRA - syncdata.telescopeRA;
            syncdata.deltaDEC     = syncdata.targetDEC - syncdata.telescopeDEC;
            IDSetNumber(StandardSyncPointNP, NULL);
            IUFindNumber(StandardSyncNP, "STANDARDSYNC_RA")->value = syncdata.deltaRA;
            IUFindNumber(StandardSyncNP, "STANDARDSYNC_DE")->value = syncdata.deltaDEC;
            IDSetNumber(StandardSyncNP, NULL);

            DEBUGF(INDI::Logger::DBG_SESSION, "Mount manually Synced (deltaRA = %.6f deltaDEC = %.6f)",
                   syncdata.deltaRA, syncdata.deltaDEC);
            //IDLog("Mount Synced (deltaRA = %.6f deltaDEC = %.6f)\n", syncdata.deltaRA, syncdata.deltaDEC);
            if (syncdata2.lst != 0.0)
            {
                computePolarAlign(syncdata2, syncdata, getLatitude(), &tpa_alt, &tpa_az);
                IUFindNumber(SyncPolarAlignNP, "SYNCPOLARALIGN_ALT")->value = tpa_alt;
                IUFindNumber(SyncPolarAlignNP, "SYNCPOLARALIGN_AZ")->value  = tpa_az;
                IDSetNumber(SyncPolarAlignNP, NULL);
                IDLog("computePolarAlign: Telescope Polar Axis: alt = %g, az = %g\n", tpa_alt, tpa_az);
            }

            return true;
        }
    }

#ifdef WITH_ALIGN_GEEHALEL
    if (align)
    {
        compose = align->ISNewNumber(dev, name, values, names, n);
        if (compose)
            return true;
    }
#endif

    if (simulator)
    {
        compose = simulator->ISNewNumber(dev, name, values, names, n);
        if (compose)
            return true;
    }

#ifdef WITH_SCOPE_LIMITS
    if (horizon)
    {
        compose = horizon->ISNewNumber(dev, name, values, names, n);
        if (compose)
            return true;
    }
#endif

#ifdef WITH_ALIGN
    ProcessAlignmentNumberProperties(this, name, values, names, n);
#endif
    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool EQMod::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    bool compose = true;
    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, "SIMULATION"))
        {
            ISwitchVectorProperty *svp = getSwitch(name);

            IUUpdateSwitch(svp, states, names, n);
            ISwitch *sp = IUFindOnSwitch(svp);
            if (!sp)
                return false;

            if (isConnected())
            {
                DEBUG(INDI::Logger::DBG_WARNING,
                      "Mount must be disconnected before you can change simulation settings.");
                svp->s = IPS_ALERT;
                IDSetSwitch(svp, NULL);
                return false;
            }

            if (!strcmp(sp->name, "ENABLE"))
                setStepperSimulation(true);
            else
                setStepperSimulation(false);
            return true;
        }

        if (strcmp(name, "USEBACKLASH") == 0)
        {
            IUUpdateSwitch(UseBacklashSP, states, names, n);
            mount->SetBacklashUseRA((IUFindSwitch(UseBacklashSP, "USEBACKLASHRA")->s == ISS_ON ? true : false));
            mount->SetBacklashUseDE((IUFindSwitch(UseBacklashSP, "USEBACKLASHDE")->s == ISS_ON ? true : false));
            DEBUGF(INDI::Logger::DBG_SESSION, "Use Backlash :  RA: %s, DE: %s",
                   IUFindSwitch(UseBacklashSP, "USEBACKLASHRA")->s == ISS_ON ? "True" : "False",
                   IUFindSwitch(UseBacklashSP, "USEBACKLASHDE")->s == ISS_ON ? "True" : "False");
            UseBacklashSP->s = IPS_IDLE;
            IDSetSwitch(UseBacklashSP, NULL);
            return true;
        }

        if (strcmp(name, "TRACKDEFAULT") == 0)
        {
            ISwitch *swbefore, *swafter;
            swbefore = IUFindOnSwitch(TrackDefaultSP);
            IUUpdateSwitch(TrackDefaultSP, states, names, n);
            swafter = IUFindOnSwitch(TrackDefaultSP);
            if (swbefore != swafter)
            {
                TrackDefaultSP->s = IPS_IDLE;
                IDSetSwitch(TrackDefaultSP, NULL);
                DEBUGF(INDI::Logger::DBG_SESSION, "Changed Track Default (from %s to %s).", swbefore->name,
                       swafter->name);
            }
            return true;
        }

        if (strcmp(name, "ST4_GUIDE_RATE_WE") == 0)
        {
            ISwitch *swbefore, *swafter;
            swbefore = IUFindOnSwitch(ST4GuideRateWESP);
            IUUpdateSwitch(ST4GuideRateWESP, states, names, n);
            swafter = IUFindOnSwitch(ST4GuideRateWESP);
            if (swbefore != swafter)
            {
                unsigned char rate = '0' + (unsigned char)IUFindOnSwitchIndex(ST4GuideRateWESP);
                mount->SetST4RAGuideRate(rate);
                ST4GuideRateWESP->s = IPS_IDLE;
                IDSetSwitch(ST4GuideRateWESP, NULL);
                DEBUGF(INDI::Logger::DBG_SESSION, "Changed ST4 Guide rate WE (from %s to %s).", swbefore->label,
                       swafter->label);
            }
            return true;
        }

        if (strcmp(name, "ST4_GUIDE_RATE_NS") == 0)
        {
            ISwitch *swbefore, *swafter;
            swbefore = IUFindOnSwitch(ST4GuideRateNSSP);
            IUUpdateSwitch(ST4GuideRateNSSP, states, names, n);
            swafter = IUFindOnSwitch(ST4GuideRateNSSP);
            if (swbefore != swafter)
            {
                unsigned char rate = '0' + (unsigned char)IUFindOnSwitchIndex(ST4GuideRateNSSP);
                mount->SetST4DEGuideRate(rate);
                ST4GuideRateNSSP->s = IPS_IDLE;
                IDSetSwitch(ST4GuideRateNSSP, NULL);
                DEBUGF(INDI::Logger::DBG_SESSION, "Changed ST4 Guide rate NS (from %s to %s).", swbefore->label,
                       swafter->label);
            }
            return true;
        }

        if (!strcmp(name, "SYNCMANAGE"))
        {
            ISwitchVectorProperty *svp = getSwitch(name);
            IUUpdateSwitch(svp, states, names, n);
            ISwitch *sp = IUFindOnSwitch(svp);
            if (!sp)
                return false;
            IDSetSwitch(svp, NULL);

            if (!strcmp(sp->name, "SYNCCLEARDELTA"))
            {
                bzero(&syncdata, sizeof(syncdata));
                bzero(&syncdata2, sizeof(syncdata2));
                IUFindNumber(StandardSyncNP, "STANDARDSYNC_RA")->value = syncdata.deltaRA;
                IUFindNumber(StandardSyncNP, "STANDARDSYNC_DE")->value = syncdata.deltaDEC;
                IDSetNumber(StandardSyncNP, NULL);
                IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_JD")->value           = syncdata.jd;
                IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_SYNCTIME")->value     = syncdata.lst;
                IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_CELESTIAL_RA")->value = syncdata.targetRA;
                ;
                IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_CELESTIAL_DE")->value = syncdata.targetDEC;
                ;
                IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_TELESCOPE_RA")->value = syncdata.telescopeRA;
                ;
                IUFindNumber(StandardSyncPointNP, "STANDARDSYNCPOINT_TELESCOPE_DE")->value = syncdata.telescopeDEC;
                ;
                IDSetNumber(StandardSyncPointNP, NULL);
                DEBUG(INDI::Logger::DBG_SESSION, "Cleared current Sync Data");
                tpa_alt                                                     = 0.0;
                tpa_az                                                      = 0.0;
                IUFindNumber(SyncPolarAlignNP, "SYNCPOLARALIGN_ALT")->value = tpa_alt;
                IUFindNumber(SyncPolarAlignNP, "SYNCPOLARALIGN_AZ")->value  = tpa_az;
                IDSetNumber(SyncPolarAlignNP, NULL);
                return true;
            }
        }

        if (!strcmp(name, "REVERSEDEC"))
        {
            IUUpdateSwitch(ReverseDECSP, states, names, n);

            ReverseDECSP->s = IPS_OK;

            DEInverted = (ReverseDECSP->sp[0].s == ISS_ON) ? true : false;

            DEBUG(INDI::Logger::DBG_SESSION, "Inverting Declination Axis.");

            IDSetSwitch(ReverseDECSP, NULL);
        }

        //if (MountInformationTP && MountInformationTP->tp && (!strcmp(MountInformationTP->tp[0].text, "EQ8") || !strcmp(MountInformationTP->tp[0].text, "AZEQ6"))) {
        if (mount->HasHomeIndexers())
        {
            if (AutoHomeSP && strcmp(name, AutoHomeSP->name) == 0)
            {
                if ((TrackState != SCOPE_IDLE) && (TrackState != SCOPE_AUTOHOMING))
                {
                    if (TrackState != SCOPE_AUTOHOMING)
                    {
                        AutoHomeSP->s = IPS_IDLE;
                        IUResetSwitch(AutoHomeSP);
                        IDSetSwitch(AutoHomeSP, NULL);
                    }
                    DEBUG(INDI::Logger::DBG_WARNING, "Can not start AutoHome. Scope not idle");
                    return true;
                }

                if (TrackState == SCOPE_AUTOHOMING)
                {
                    AutoHomeSP->s = IPS_IDLE;
                    IUResetSwitch(AutoHomeSP);
                    IDSetSwitch(AutoHomeSP, NULL);
                    DEBUG(INDI::Logger::DBG_WARNING, "Aborting AutoHome.");
                    Abort();
                    return true;
                }

                if (AutohomeState == AUTO_HOME_IDLE)
                {
                    AutoHomeSP->s = IPS_OK;
                    IUResetSwitch(AutoHomeSP);
                    IDSetSwitch(AutoHomeSP, NULL);
                    DEBUG(INDI::Logger::DBG_WARNING, "*** AutoHome NOT TESTED. Press PERFORM AGAIN TO CONFIRM. ***");
                    AutohomeState      = AUTO_HOME_CONFIRM;
                    ah_confirm_timeout = 10;
                    return true;
                }
                if (AutohomeState == AUTO_HOME_CONFIRM)
                {
                    IUUpdateSwitch(AutoHomeSP, states, names, n);
                    AutoHomeSP->s = IPS_BUSY;
                    DEBUG(INDI::Logger::DBG_SESSION, "Starting Autohome.");
                    IDSetSwitch(AutoHomeSP, NULL);
                    TrackState = SCOPE_AUTOHOMING;
                    try
                    {
                        DEBUG(INDI::Logger::DBG_SESSION, "AutoHome phase 1: turning off aux encoders");
                        mount->TurnRAEncoder(false);
                        mount->TurnDEEncoder(false);
                        DEBUG(INDI::Logger::DBG_SESSION, "AutoHome phase 1: resetting home position indexes");
                        mount->ResetRAIndexer();
                        mount->ResetDEIndexer();
                        DEBUG(INDI::Logger::DBG_SESSION,
                              "AutoHome phase 1: reading home position indexes to set directions");
                        mount->GetRAIndexer();
                        mount->GetDEIndexer();
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 1: read home position indexes: RA=0x%x DE=0x%x",
                               mount->GetlastreadRAIndexer(), mount->GetlastreadDEIndexer());
                        if (mount->GetlastreadRAIndexer() == 0)
                            ah_bSlewingUp_RA = true;
                        else
                            ah_bSlewingUp_RA = false;
                        if (mount->GetlastreadDEIndexer() == 0)
                            ah_bSlewingUp_DE = true;
                        else
                            ah_bSlewingUp_DE = false;
                        ah_iPosition_RA = mount->GetRAEncoder();
                        ah_iPosition_DE = mount->GetDEEncoder();
                        ah_iChanges     = (5 * mount->GetRAEncoderTotal()) / 360;
                        if (ah_bSlewingUp_RA)
                            ah_iPosition_RA = ah_iPosition_RA - ah_iChanges;
                        else
                            ah_iPosition_RA = ah_iPosition_RA + ah_iChanges;
                        ah_iChanges = (5 * mount->GetDEEncoderTotal()) / 360;
                        if (ah_bSlewingUp_DE)
                            ah_iPosition_DE = ah_iPosition_DE - ah_iChanges;
                        else
                            ah_iPosition_DE = ah_iPosition_DE + ah_iChanges;
                        DEBUG(INDI::Logger::DBG_SESSION,
                              "AutoHome phase 1: trying to move further away from home position");
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "AutoHome phase 1: slewing to RA=0x%x (up=%c) DE=0x%x (up=%c)", ah_iPosition_RA,
                               (ah_bSlewingUp_RA ? '1' : '0'), ah_iPosition_DE, (ah_bSlewingUp_DE ? '1' : '0'));
                        mount->AbsSlewTo(ah_iPosition_RA, ah_iPosition_DE, ah_bSlewingUp_RA, ah_bSlewingUp_DE);
                        AutohomeState = AUTO_HOME_WAIT_PHASE1;
                    }
                    catch (EQModError e)
                    {
                        AutoHomeSP->s = IPS_ALERT;
                        IUResetSwitch(AutoHomeSP);
                        IDSetSwitch(AutoHomeSP, NULL);
                        AutohomeState = AUTO_HOME_IDLE;
                        TrackState    = SCOPE_IDLE;
                        return (e.DefaultHandleException(this));
                    }
                }
            }
        }

        if (mount->HasAuxEncoders())
        {
            if (AuxEncoderSP && strcmp(name, AuxEncoderSP->name) == 0)
            {
                IUUpdateSwitch(AuxEncoderSP, states, names, n);
                if (AuxEncoderSP->sp[1].s == ISS_ON)
                {
                    AuxEncoderSP->s = IPS_OK;
                    DEBUG(INDI::Logger::DBG_DEBUG, "Turning auxiliary encoders on.");
                    mount->TurnRAEncoder(true);
                    mount->TurnDEEncoder(true);
                }
                else
                {
                    AuxEncoderSP->s = IPS_IDLE;
                    DEBUG(INDI::Logger::DBG_DEBUG, "Turning auxiliary encoders off.");
                    mount->TurnRAEncoder(false);
                    mount->TurnDEEncoder(false);
                }
                IDSetSwitch(AuxEncoderSP, NULL);
            }
        }

        if (mount->HasPPEC())
        {
            if (RAPPECTrainingSP && strcmp(name, RAPPECTrainingSP->name) == 0)
            {
                IUUpdateSwitch(RAPPECTrainingSP, states, names, n);
                if (RAPPECTrainingSP->sp[1].s == ISS_ON)
                {
                    if (TrackState != SCOPE_TRACKING)
                    {
                        RAPPECTrainingSP->s = IPS_IDLE;
                        DEBUG(INDI::Logger::DBG_WARNING, "Can not start RA PPEC Training. Scope not tracking");
                        IUResetSwitch(RAPPECTrainingSP);
                        RAPPECTrainingSP->sp[0].s = ISS_ON;
                        RAPPECTrainingSP->sp[1].s = ISS_OFF;
                    }
                    else
                    {
                        RAPPECTrainingSP->s = IPS_BUSY;
                        DEBUG(INDI::Logger::DBG_SESSION, "Turning RA PPEC Training on.");
                        try
                        {
                            mount->TurnRAPPECTraining(true);
                        }
                        catch (EQModError e)
                        {
                            DEBUG(INDI::Logger::DBG_WARNING, "Unable to start RA PPEC Training.");
                            RAPPECTrainingSP->s       = IPS_ALERT;
                            RAPPECTrainingSP->sp[0].s = ISS_ON;
                            RAPPECTrainingSP->sp[1].s = ISS_OFF;
                        }
                    }
                }
                else
                {
                    RAPPECTrainingSP->s = IPS_IDLE;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning RA PPEC Training off.");
                    mount->TurnRAPPECTraining(false);
                }
                IDSetSwitch(RAPPECTrainingSP, NULL);
                return true;
            }
            if (RAPPECSP && strcmp(name, RAPPECSP->name) == 0)
            {
                IUUpdateSwitch(RAPPECSP, states, names, n);
                if (RAPPECSP->sp[1].s == ISS_ON)
                {
                    RAPPECSP->s = IPS_BUSY;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning RA PPEC on.");
                    mount->TurnRAPPEC(true);
                }
                else
                {
                    RAPPECSP->s = IPS_IDLE;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning RA PPEC off.");
                    mount->TurnRAPPEC(false);
                }
                IDSetSwitch(RAPPECSP, NULL);
                return true;
            }
            if (DEPPECTrainingSP && strcmp(name, DEPPECTrainingSP->name) == 0)
            {
                IUUpdateSwitch(DEPPECTrainingSP, states, names, n);
                if (DEPPECTrainingSP->sp[1].s == ISS_ON)
                {
                    if (TrackState != SCOPE_TRACKING)
                    {
                        DEPPECTrainingSP->s = IPS_IDLE;
                        DEBUG(INDI::Logger::DBG_WARNING, "Can not start DEC PPEC Training. Scope not tracking");
                        IUResetSwitch(DEPPECTrainingSP);
                        DEPPECTrainingSP->sp[0].s = ISS_ON;
                        DEPPECTrainingSP->sp[1].s = ISS_OFF;
                    }
                    else
                    {
                        DEPPECTrainingSP->s = IPS_BUSY;
                        DEBUG(INDI::Logger::DBG_SESSION, "Turning DEC PPEC Training on.");
                        try
                        {
                            mount->TurnDEPPECTraining(true);
                        }
                        catch (EQModError e)
                        {
                            DEBUG(INDI::Logger::DBG_WARNING, "Unable to start DEC PPEC Training.");
                            DEPPECTrainingSP->s       = IPS_ALERT;
                            DEPPECTrainingSP->sp[0].s = ISS_ON;
                            DEPPECTrainingSP->sp[1].s = ISS_OFF;
                        }
                    }
                }
                else
                {
                    DEPPECTrainingSP->s = IPS_IDLE;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning DEC PPEC Training off.");
                    mount->TurnDEPPECTraining(false);
                }
                IDSetSwitch(DEPPECTrainingSP, NULL);
                return true;
            }
            if (DEPPECSP && strcmp(name, DEPPECSP->name) == 0)
            {
                IUUpdateSwitch(DEPPECSP, states, names, n);
                if (DEPPECSP->sp[1].s == ISS_ON)
                {
                    DEPPECSP->s = IPS_BUSY;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning DEC PPEC on.");
                    mount->TurnDEPPEC(true);
                }
                else
                {
                    DEPPECSP->s = IPS_IDLE;
                    DEBUG(INDI::Logger::DBG_SESSION, "Turning DEC PPEC off.");
                    mount->TurnDEPPEC(false);
                }
                IDSetSwitch(DEPPECSP, NULL);
                return true;
            }
        }

#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
        if (AlignSyncModeSP && strcmp(name, AlignSyncModeSP->name) == 0)
        {
            ISwitch *sw;
            AlignSyncModeSP->s = IPS_OK;
            IUUpdateSwitch(AlignSyncModeSP, states, names, n);
            //for (int i=0; i < n; i++)
            //  IDLog("AlignSyncMode Switch %s %d\n", names[i], states[i]);
            sw = IUFindOnSwitch(AlignSyncModeSP);
            IDSetSwitch(AlignSyncModeSP, "Sync mode set to %s", sw->label);
            return true;
        }
#endif

#if defined WITH_ALIGN_GEEHALEL && defined WITH_ALIGN
        if (strcmp(name, AlignMethodSP.name) == 0)
        {
            ISwitch *sw;
            AlignMethodSP.s = IPS_OK;
            IUUpdateSwitch(&AlignMethodSP, states, names, n);
            //for (int i=0; i < n; i++)
            //  IDLog("AlignSyncMode Switch %s %d\n", names[i], states[i]);
            sw = IUFindOnSwitch(&AlignMethodSP);
            IDSetSwitch(&AlignMethodSP, "Align method set to %s", sw->label);
            return true;
        }
#endif
    }
#ifdef WITH_ALIGN_GEEHALEL
    if (align)
    {
        compose = align->ISNewSwitch(dev, name, states, names, n);
        if (compose)
            return true;
    }
#endif

    if (simulator)
    {
        compose = simulator->ISNewSwitch(dev, name, states, names, n);
        if (compose)
            return true;
    }
#ifdef WITH_SCOPE_LIMITS
    if (horizon)
    {
        compose = horizon->ISNewSwitch(dev, name, states, names, n);
        if (compose)
            return true;
    }
#endif
#ifdef WITH_ALIGN
    ProcessAlignmentSwitchProperties(this, name, states, names, n);
#endif

    INDI::Logger::ISNewSwitch(dev, name, states, names, n);

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool EQMod::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    bool compose;
#ifdef WITH_ALIGN_GEEHALEL
    if (align)
    {
        compose = align->ISNewText(dev, name, texts, names, n);
        if (compose)
            return true;
    }
#endif
    if (simulator)
    {
        compose = simulator->ISNewText(dev, name, texts, names, n);
        if (compose)
            return true;
    }
#ifdef WITH_SCOPE_LIMITS
    if (horizon)
    {
        compose = horizon->ISNewText(dev, name, texts, names, n);
        if (compose)
            return true;
    }
#endif
#ifdef WITH_ALIGN
    ProcessAlignmentTextProperties(this, name, texts, names, n);
#endif
    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

#ifdef WITH_ALIGN
bool EQMod::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
                      char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentBLOBProperties(this, name, sizes, blobsizes, blobs, formats, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}
#endif

bool EQMod::updateTime(ln_date *lndate_utc, double utc_offset)
{
    char utc_time[32];
    lndate.seconds = lndate_utc->seconds;
    lndate.minutes = lndate_utc->minutes;
    lndate.hours   = lndate_utc->hours;
    lndate.days    = lndate_utc->days;
    lndate.months  = lndate_utc->months;
    lndate.years   = lndate_utc->years;

    utc.tm_sec  = lndate.seconds;
    utc.tm_min  = lndate.minutes;
    utc.tm_hour = lndate.hours;
    utc.tm_mday = lndate.days;
    utc.tm_mon  = lndate.months - 1;
    utc.tm_year = lndate.years - 1900;

    gettimeofday(&lasttimeupdate, NULL);
    get_utc_time(&lastclockupdate);

    strftime(utc_time, 32, "%Y-%m-%dT%H:%M:%S", &utc);

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting UTC Time to %s, Offset %g", utc_time, utc_offset);

    return true;
}

double EQMod::GetRASlew()
{
    ISwitch *sw;
    double rate = 1.0;
    sw          = IUFindOnSwitch(&SlewRateSP);
    if (!strcmp(sw->name, "SLEWCUSTOM"))
        rate = IUFindNumber(SlewSpeedsNP, "RASLEW")->value;
    else
        rate = *((double *)sw->aux);
    return rate;
}

double EQMod::GetDESlew()
{
    ISwitch *sw;
    double rate = 1.0;
    sw          = IUFindOnSwitch(&SlewRateSP);
    if (!strcmp(sw->name, "SLEWCUSTOM"))
        rate = IUFindNumber(SlewSpeedsNP, "DESLEW")->value;
    else
        rate = *((double *)sw->aux);
    return rate;
}

bool EQMod::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    const char *dirStr = (dir == DIRECTION_NORTH) ? "North" : "South";
    double rate        = (dir == DIRECTION_NORTH) ? GetDESlew() : GetDESlew() * -1;

    try
    {
        switch (command)
        {
        case MOTION_START:
            if (gotoInProgress() || (TrackState == SCOPE_PARKING) || (TrackState == SCOPE_PARKED))
            {
                DEBUG(INDI::Logger::DBG_WARNING, "Can not slew while goto/park in progress, or scope parked.");
                return false;
            }

            DEBUGF(INDI::Logger::DBG_SESSION, "Starting %s slew.", dirStr);
            if (DEInverted)
                rate = -rate;
            mount->SlewDE(rate);
            TrackState = SCOPE_SLEWING;
            break;

        case MOTION_STOP:
            DEBUGF(INDI::Logger::DBG_SESSION, "%s Slew stopped", dirStr);
            mount->StopDE();
            //if (TrackModeSP->s == IPS_BUSY)
            if (RememberTrackState == SCOPE_TRACKING)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Restarting DE Tracking...");
                TrackState = SCOPE_TRACKING;
                mount->StartDETracking(GetDETrackRate());
            }
            else
                TrackState = SCOPE_IDLE;

            break;
        }
    }
    catch (EQModError e)
    {
        return e.DefaultHandleException(this);
    }
    return true;
}

bool EQMod::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    const char *dirStr = (dir == DIRECTION_WEST) ? "West" : "East";
    double rate        = (dir == DIRECTION_WEST) ? GetRASlew() : GetRASlew() * -1;

    try
    {
        switch (command)
        {
        case MOTION_START:
            if (gotoInProgress() || (TrackState == SCOPE_PARKING) || (TrackState == SCOPE_PARKED))
            {
                DEBUG(INDI::Logger::DBG_WARNING, "Can not slew while goto/park in progress, or scope parked.");
                return false;
            }

            DEBUGF(INDI::Logger::DBG_SESSION, "Starting %s slew.", dirStr);
            if (RAInverted)
                rate = -rate;
            mount->SlewRA(rate);
            TrackState = SCOPE_SLEWING;
            break;

        case MOTION_STOP:
            DEBUGF(INDI::Logger::DBG_SESSION, "%s Slew stopped", dirStr);
            mount->StopRA();
            //if (TrackModeSP->s == IPS_BUSY)
            if (RememberTrackState == SCOPE_TRACKING)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Restarting RA Tracking...");
                TrackState = SCOPE_TRACKING;
                mount->StartRATracking(GetRATrackRate());
            }
            else
                TrackState = SCOPE_IDLE;
            break;
        }
    }
    catch (EQModError e)
    {
        return e.DefaultHandleException(this);
    }
    return true;
}

bool EQMod::Abort()
{
    try
    {
        mount->StopRA();
    }
    catch (EQModError e)
    {
        if (!(e.DefaultHandleException(this)))
        {
            DEBUG(INDI::Logger::DBG_WARNING, "Abort: error while stopping RA motor");
        }
    }
    try
    {
        mount->StopDE();
    }
    catch (EQModError e)
    {
        if (!(e.DefaultHandleException(this)))
        {
            DEBUG(INDI::Logger::DBG_WARNING, "Abort: error while stopping DE motor");
        }
    }

    GuideNSNP.s = IPS_IDLE;
    IDSetNumber(&GuideNSNP, NULL);
    GuideWENP.s = IPS_IDLE;
    IDSetNumber(&GuideWENP, NULL);
#if 0
    TrackModeSP->s = IPS_IDLE;
    IUResetSwitch(TrackModeSP);
    IDSetSwitch(TrackModeSP, NULL);
#endif
    AutohomeState = AUTO_HOME_IDLE;
    AutoHomeSP->s = IPS_IDLE;
    IUResetSwitch(AutoHomeSP);
    IDSetSwitch(AutoHomeSP, NULL);
    TrackState = SCOPE_IDLE;
    if (gotoparams.completed == false)
        gotoparams.completed = true;

    return true;
}

void EQMod::timedguideNSCallback(void *userpointer)
{
    EQMod *p = ((EQMod *)userpointer);
    try
    {
        if (p->mount->HasPPEC())
        {
            if (p->restartguideDEPPEC)
            {
                p->restartguideDEPPEC = false;
                DEBUGDEVICE(p->getDeviceName(), INDI::Logger::DBG_SESSION, "Turning DEC PPEC on after guiding.");
                p->mount->TurnDEPPEC(true);
            }
        }
        p->mount->StartDETracking(p->GetDETrackRate());
    }
    catch (EQModError e)
    {
        if (!(e.DefaultHandleException(p)))
        {
            DEBUGDEVICE(p->getDeviceName(), INDI::Logger::DBG_WARNING,
                        "Timed guide North/South Error: can not restart tracking");
        }
    }
    p->GuideComplete(AXIS_DE);
    DEBUGDEVICE(p->getDeviceName(), INDI::Logger::DBG_DEBUG, "End Timed guide North/South");
    IERmTimer(p->GuideTimerNS);
}

void EQMod::timedguideWECallback(void *userpointer)
{
    EQMod *p = ((EQMod *)userpointer);
    try
    {
        if (p->mount->HasPPEC())
        {
            if (p->restartguideRAPPEC)
            {
                p->restartguideRAPPEC = false;
                DEBUGDEVICE(p->getDeviceName(), INDI::Logger::DBG_SESSION, "Turning RA PPEC on after guiding.");
                p->mount->TurnRAPPEC(true);
            }
        }
        p->mount->StartRATracking(p->GetRATrackRate());
    }
    catch (EQModError e)
    {
        if (!(e.DefaultHandleException(p)))
        {
            DEBUGDEVICE(p->getDeviceName(), INDI::Logger::DBG_WARNING,
                        "Timed guide West/East Error: can not restart tracking");
        }
    }
    p->GuideComplete(AXIS_RA);
    DEBUGDEVICE(p->getDeviceName(), INDI::Logger::DBG_SESSION, "End Timed guide West/East");
    IERmTimer(p->GuideTimerWE);
}

void EQMod::computePolarAlign(SyncData s1, SyncData s2, double lat, double *tpaalt, double *tpaaz)
/*
From // // http://www.whim.org/nebula/math/pdf/twostar.pdf
 */
{
    double delta1 = 0, alpha1 = 0, delta2 = 0, alpha2 = 0;
    double d1 = 0, d2 = 0; /* corrected delta1/delta2 */
    double cdelta1 = 0, calpha1 = 0, cdelta2 = 0, calpha2 = 0;
    double Delta = 0;
    double cosDelta1 = 0, cosDelta2 = 0;
    double cosd2pd1 = 0, d2pd1 = 0;
    double tpadelta = 0, tpaalpha = 0;
    double sintpadelta = 0, costpaalpha = 0, sintpaalpha = 0;
    double cosama1 = 0, cosama2 = 0;
    double cosaz = 0, sinaz = 0;
    double beta = 0;

    // Star s2 polar align
    double s2tra = 0, s2tdec = 0;
    char s2trasexa[13], s2tdecsexa[13];
    char s2rasexa[13], s2decsexa[13];

    alpha1  = ln_deg_to_rad((s1.telescopeRA - s1.lst) * 360.0 / 24.0);
    delta1  = ln_deg_to_rad(s1.telescopeDEC);
    alpha2  = ln_deg_to_rad((s2.telescopeRA - s2.lst) * 360.0 / 24.0);
    delta2  = ln_deg_to_rad(s2.telescopeDEC);
    calpha1 = ln_deg_to_rad((s1.targetRA - s1.lst) * 360.0 / 24.0);
    cdelta1 = ln_deg_to_rad(s1.targetDEC);
    calpha2 = ln_deg_to_rad((s2.targetRA - s2.lst) * 360.0 / 24.0);
    cdelta2 = ln_deg_to_rad(s2.targetDEC);

    if ((calpha2 == calpha1) || (alpha1 == alpha2))
        return;

    cosDelta1 = sin(cdelta1) * sin(cdelta2) + (cos(cdelta1) * cos(cdelta2) * cos(calpha2 - calpha1));
    cosDelta2 = sin(delta1) * sin(delta2) + (cos(delta1) * cos(delta2) * cos(alpha2 - alpha1));

    if (cosDelta1 != cosDelta2)
        DEBUGF(INDI::Logger::DBG_DEBUG,
               "PolarAlign -- Telescope axes are not perpendicular. Angular distances are:celestial=%g telescope=%g",
               acos(cosDelta1), acos(cosDelta2));
    Delta = acos(cosDelta1);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Angular distance of the two stars is %g", Delta);

    //cosd2md1 = sin(delta1) * sin(delta2) + cos(delta1) * cos(delta2);
    cosd2pd1 = ((cos(delta2 - delta1) * (1 + cos(alpha2 - alpha1))) - (2.0 * cosDelta2)) / (1 - cos(alpha2 - alpha1));
    d2pd1    = acos(cosd2pd1);
    if (delta2 * delta1 > 0.0)
    { /* same sign */
        if (delta1 < 0.0)
            d2pd1 = -d2pd1;
    }
    else
    {
        if (fabs(delta1) > fabs(delta2))
        {
            if (delta1 < 0.0)
                d2pd1 = -d2pd1;
        }
        else
        {
            if (delta2 < 0.0)
                d2pd1 = -d2pd1;
        }
    }

    d2 = (d2pd1 + delta2 - delta1) / 2.0;
    d1 = d2pd1 - d2;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Computed delta1 = %g (%g) delta2 = %g (%g)", d1, delta1, d2, delta2);

    delta1 = d1;
    delta2 = d2;

    sintpadelta =
            (sin(delta1) * sin(cdelta1)) + (sin(delta2) * sin(cdelta2)) -
            cosDelta1 * ((sin(delta1) * sin(cdelta2)) + (sin(cdelta1) * sin(delta2))) +
            (cos(delta1) * cos(delta2) * sin(alpha2 - alpha1) * cos(cdelta1) * cos(cdelta2) * sin(calpha2 - calpha1));
    sintpadelta = sintpadelta / (sin(Delta) * sin(Delta));
    tpadelta    = asin(sintpadelta);
    cosama1     = (sin(delta1) - (sin(cdelta1) * sintpadelta)) / (cos(cdelta1) * cos(tpadelta));
    cosama2     = (sin(delta2) - (sin(cdelta2) * sintpadelta)) / (cos(cdelta2) * cos(tpadelta));

    costpaalpha = (sin(calpha2) * cosama1 - sin(calpha1) * cosama2) / sin(calpha2 - calpha1);
    sintpaalpha = (cos(calpha1) * cosama2 - cos(calpha2) * cosama1) / sin(calpha2 - calpha1);
    //tpaalpha = acos(costpaalpha);
    //if (sintpaalpha < 0) tpaalpha = 2 * M_PI - tpaalpha;
    // tpadelta and tpaaplha are very near M_PI / 2 d: DON'T USE  atan2
    //tpaalpha=atan2(sintpaalpha, costpaalpha);
    tpaalpha = 2 * atan2(sintpaalpha, (1.0 + costpaalpha));
    DEBUGF(INDI::Logger::DBG_DEBUG, "Computed Telescope polar alignment (rad): delta(dec) = %g alpha(ha) = %g",
           tpadelta, tpaalpha);

    beta    = ln_deg_to_rad(lat);
    *tpaalt = asin(sin(tpadelta) * sin(beta) + (cos(tpadelta) * cos(beta) * cos(tpaalpha)));
    cosaz   = (sin(tpadelta) - (sin(*tpaalt) * sin(beta))) / (cos(*tpaalt) * cos(beta));
    sinaz   = (cos(tpadelta) * sin(tpaalpha)) / cos(*tpaalt);
    //*tpaaz = acos(cosaz);
    //if (sinaz < 0) *tpaaz = 2 * M_PI - *tpaaz;
    *tpaaz  = atan2(sinaz, cosaz);
    *tpaalt = ln_rad_to_deg(*tpaalt);
    *tpaaz  = ln_rad_to_deg(*tpaaz);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Computed Telescope polar alignment (deg): alt = %g az = %g", *tpaalt, *tpaaz);

    starPolarAlign(s2.lst, s2.targetRA, s2.targetDEC, (M_PI / 2) - tpaalpha, (M_PI / 2) - tpadelta, &s2tra, &s2tdec);
    fs_sexa(s2trasexa, s2tra, 2, 3600);
    fs_sexa(s2tdecsexa, s2tdec, 3, 3600);
    fs_sexa(s2rasexa, s2.targetRA, 2, 3600);
    fs_sexa(s2decsexa, s2.targetDEC, 3, 3600);
    DEBUGF(INDI::Logger::DBG_SESSION, "Star (RA=%s DEC=%s) Polar Align Coords: RA=%s DEC=%s", s2rasexa, s2decsexa,
           s2trasexa, s2tdecsexa);
    s2tra  = s2.targetRA + (s2.targetRA - s2tra);
    s2tdec = s2.targetDEC + (s2.targetDEC - s2tdec);
    fs_sexa(s2trasexa, s2tra, 2, 3600);
    fs_sexa(s2tdecsexa, s2tdec, 3, 3600);
    fs_sexa(s2rasexa, s2.targetRA, 2, 3600);
    fs_sexa(s2decsexa, s2.targetDEC, 3, 3600);

    DEBUGF(INDI::Logger::DBG_SESSION, "Star (RA=%s DEC=%s) Polar Align Goto: RA=%s DEC=%s", s2rasexa, s2decsexa,
           s2trasexa, s2tdecsexa);
}

void EQMod::starPolarAlign(double lst, double ra, double dec, double theta, double gamma, double *tra, double *tdec)
{
    double rotz[3][3];
    double rotx[3][3];
    double mat[3][3];

    double H;
    double Lc, Mc, Nc;

    double mra, mdec;
    double L, M, N;
    int i, j, k;

    H   = (lst - ra) * M_PI / 12.0;
    dec = dec * M_PI / 180.0;

    rotz[0][0] = cos(theta);
    rotz[0][1] = -sin(theta);
    rotz[0][2] = 0.0;
    rotz[1][0] = sin(theta);
    rotz[1][1] = cos(theta);
    rotz[1][2] = 0.0;
    rotz[2][0] = 0.0;
    rotz[2][1] = 0.0;
    rotz[2][2] = 1.0;

    rotx[0][0] = 1.0;
    rotx[0][1] = 0.0;
    rotx[0][2] = 0.0;
    rotx[1][0] = 0.0;
    rotx[1][1] = cos(gamma);
    rotx[1][2] = -sin(gamma);
    rotx[2][0] = 0.0;
    rotx[2][1] = sin(gamma);
    rotx[2][2] = cos(gamma);

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            mat[i][j] = 0.0;
            for (k = 0; k < 3; k++)
                mat[i][j] += rotx[i][k] * rotz[k][j];
        }
    }

    Lc = cos(dec) * cos(-H);
    Mc = cos(dec) * sin(-H);
    Nc = sin(dec);

    L = mat[0][0] * Lc + mat[0][1] * Mc + mat[0][2] * Nc;
    M = mat[1][0] * Lc + mat[1][1] * Mc + mat[1][2] * Nc;
    N = mat[2][0] * Lc + mat[2][1] * Mc + mat[2][2] * Nc;

    mra = atan2(M, L) * 12.0 / M_PI;
    //mra=atan(M/L) * 12.0 / M_PI;
    //printf("atan(M/L) %g L=%g M=%g N=%g\n", mra, L, M, N);
    //if (L < 0.0) mra = 12.0 + mra;
    mra += lst;
    while (mra < 0.0)
        mra += 24.0;
    while (mra > 24.0)
        mra -= 24.0;
    mdec  = asin(N) * 180.0 / M_PI;
    *tra  = mra;
    *tdec = mdec;
}

bool EQMod::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);
    // JM: INDI Longitude is 0 to 360 increasing EAST. libnova East is Positive, West is negative
    lnobserver.lng = longitude;

    if (lnobserver.lng > 180)
        lnobserver.lng -= 360;
    lnobserver.lat = latitude;
    if (latitude < 0.0)
        SetSouthernHemisphere(true);
    else
        SetSouthernHemisphere(false);
#ifdef WITH_ALIGN
    INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers::UpdateLocation(latitude, longitude, elevation);
    // Set this according to mount type
    SetApproximateMountAlignmentFromMountType(EQUATORIAL);
#endif
    DEBUGF(INDI::Logger::DBG_SESSION, "updateLocation: long = %g lat = %g", lnobserver.lng, lnobserver.lat);
    return true;
}

bool EQMod::SetCurrentPark()
{
    parkRAEncoder = currentRAEncoder;
    parkDEEncoder = currentDEEncoder;
    SetAxis1Park(parkRAEncoder);
    SetAxis2Park(parkDEEncoder);
    DEBUGF(INDI::Logger::DBG_SESSION, "Setting Park Position to current- RA Encoder=%ld DE Encoder=%ld", parkRAEncoder,
           parkDEEncoder);

    return true;
}

bool EQMod::SetDefaultPark()
{
    parkRAEncoder = GetAxis1ParkDefault();
    parkDEEncoder = GetAxis2ParkDefault();
    SetAxis1Park(parkRAEncoder);
    SetAxis2Park(parkDEEncoder);
    DEBUGF(INDI::Logger::DBG_SESSION, "Setting Park Position to default- RA Encoder=%ld DE Encoder=%ld", parkRAEncoder,
           parkDEEncoder);

    return true;
}

bool EQMod::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);

    if (BacklashNP)
        IUSaveConfigNumber(fp, BacklashNP);
    if (UseBacklashSP)
        IUSaveConfigSwitch(fp, UseBacklashSP);
    if (GuideRateNP)
        IUSaveConfigNumber(fp, GuideRateNP);
    if (SlewSpeedsNP)
        IUSaveConfigNumber(fp, SlewSpeedsNP);

#ifdef WITH_ALIGN_GEEHALEL
    if (align)
        align->saveConfigItems(fp);
#endif
    return true;
}

bool EQMod::SetTrackRate(double raRate, double deRate)
{
    try
    {
        mount->SetRARate(raRate / SKYWATCHER_STELLAR_SPEED);
        mount->SetDERate(deRate / SKYWATCHER_STELLAR_SPEED);
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting Custom Tracking Rates - RA=%.6f  DE=%.6f arcsec/s", raRate, deRate);

    return true;
}

bool EQMod::SetTrackMode(uint8_t mode)
{
    // GetRATrackRate..etc al already check TrackModeSP to obtain the appropiate tracking rate, so no need for mode here.
    INDI_UNUSED(mode);

    try
    {
        mount->StartRATracking(GetRATrackRate());
        mount->StartDETracking(GetDETrackRate());
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

    return true;
}

bool EQMod::SetTrackEnabled(bool enabled)
{
    try
    {
        if (enabled)
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "Start Tracking (%s).", IUFindOnSwitch(&TrackModeSP)->label);
            TrackState     = SCOPE_TRACKING;
            mount->StartRATracking(GetRATrackRate());
            mount->StartDETracking(GetDETrackRate());
        }
        else if (enabled == false)
        {
            DEBUGF(INDI::Logger::DBG_WARNING, "Stopping Tracking (%s).", IUFindOnSwitch(&TrackModeSP)->label);
            TrackState     = SCOPE_IDLE;
            mount->StopRA();
            mount->StopDE();
        }
    }
    catch (EQModError e)
    {
        return (e.DefaultHandleException(this));
    }

    return true;
}
