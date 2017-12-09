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
*/

#include "skywatcher.h"

#include "eqmod.h"

#include <indicom.h>

#include <termios.h>
#include <cmath>
#include <cstring>

extern int DBG_SCOPE_STATUS;
extern int DBG_COMM;
extern int DBG_MOUNT;

Skywatcher::Skywatcher(EQMod *t)
{
    debug         = false;
    debugnextread = false;
    simulation    = false;
    telescope     = t;
    reconnect     = false;
}

Skywatcher::~Skywatcher(void)
{
    Disconnect();
}

void Skywatcher::setDebug(bool enable)
{
    debug = enable;
}
bool Skywatcher::isDebug()
{
    return debug;
}

void Skywatcher::setPortFD(int value)
{
    PortFD = value;
}

void Skywatcher::setSimulation(bool enable)
{
    simulation = enable;
}
bool Skywatcher::isSimulation()
{
    return simulation;
}

const char *Skywatcher::getDeviceName()
{
    return telescope->getDeviceName();
}

/* API */

bool Skywatcher::Handshake()
{
    if (isSimulation())
    {
        telescope->simulator->Connect();
        return true;
    }

    unsigned long tmpMCVersion = 0;

    dispatch_command(InquireMotorBoardVersion, Axis1, NULL);
    read_eqmod();
    tmpMCVersion = Revu24str2long(response + 1);
    MCVersion    = ((tmpMCVersion & 0xFF) << 16) | ((tmpMCVersion & 0xFF00)) | ((tmpMCVersion & 0xFF0000) >> 16);
    MountCode    = MCVersion & 0xFF;
    /* Check supported mounts here */
    if ((MountCode == 0x80) || (MountCode == 0x81) /*|| (MountCode == 0x82)*/ || (MountCode == 0x90))
    {
        throw EQModError(EQModError::ErrDisconnect,
                         "Mount not supported: mount code 0x%x (0x80=GT, 0x81=MF, 0x82=114GT, 0x90=DOB)", MountCode);
        return false;
    }

    return true;
}

bool Skywatcher::Disconnect()
{
    if (PortFD < 0)
        return true;
    StopMotor(Axis1);
    StopMotor(Axis2);
    // Deactivate motor (for geehalel mount only)
    /*
  if (MountCode == 0xF0) {
    dispatch_command(Deactivate, Axis1, NULL);
    read_eqmod();
  }
  */
    return true;
}

unsigned long Skywatcher::GetRAEncoder()
{
    // Axis Position
    dispatch_command(GetAxisPosition, Axis1, NULL);
    read_eqmod();
    RAStep = Revu24str2long(response + 1);
    gettimeofday(&lastreadmotorposition[Axis1], NULL);
    DEBUGF(DBG_SCOPE_STATUS, "%s() = %ld", __FUNCTION__, RAStep);
    return RAStep;
}

unsigned long Skywatcher::GetDEEncoder()
{
    // Axis Position
    dispatch_command(GetAxisPosition, Axis2, NULL);
    read_eqmod();

    DEStep = Revu24str2long(response + 1);
    gettimeofday(&lastreadmotorposition[Axis2], NULL);
    DEBUGF(DBG_SCOPE_STATUS, "%s() = %ld", __FUNCTION__, DEStep);
    return DEStep;
}

unsigned long Skywatcher::GetRAEncoderZero()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %ld", __FUNCTION__, RAStepInit);
    return RAStepInit;
}

unsigned long Skywatcher::GetRAEncoderTotal()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %ld", __FUNCTION__, RASteps360);
    return RASteps360;
}

unsigned long Skywatcher::GetRAEncoderHome()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %ld", __FUNCTION__, RAStepHome);
    return RAStepHome;
}

unsigned long Skywatcher::GetDEEncoderZero()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %ld", __FUNCTION__, DEStepInit);
    return DEStepInit;
}

unsigned long Skywatcher::GetDEEncoderTotal()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %ld", __FUNCTION__, DESteps360);
    return DESteps360;
}

unsigned long Skywatcher::GetDEEncoderHome()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %ld", __FUNCTION__, DEStepHome);
    return DEStepHome;
}

unsigned long Skywatcher::GetRAPeriod()
{
    DEBUGF(DBG_SCOPE_STATUS, "%s() = %ld", __FUNCTION__, RAPeriod);
    return RAPeriod;
}

unsigned long Skywatcher::GetDEPeriod()
{
    DEBUGF(DBG_SCOPE_STATUS, "%s() = %ld", __FUNCTION__, DEPeriod);
    return DEPeriod;
}

unsigned long Skywatcher::GetlastreadRAIndexer()
{
    if (MountCode != 0x04 && MountCode != 0x05)
        throw EQModError(EQModError::ErrInvalidCmd, "Incorrect mount type");
    DEBUGF(DBG_SCOPE_STATUS, "%s() = %ld", __FUNCTION__, lastreadIndexer[Axis1]);
    return lastreadIndexer[Axis1];
}

unsigned long Skywatcher::GetlastreadDEIndexer()
{
    if (MountCode != 0x04 && MountCode != 0x05)
        throw EQModError(EQModError::ErrInvalidCmd, "Incorrect mount type");
    DEBUGF(DBG_SCOPE_STATUS, "%s() = %ld", __FUNCTION__, lastreadIndexer[Axis2]);
    return lastreadIndexer[Axis2];
}

void Skywatcher::GetRAMotorStatus(ILightVectorProperty *motorLP)
{
    ReadMotorStatus(Axis1);
    if (!RAInitialized)
    {
        IUFindLight(motorLP, "RAInitialized")->s = IPS_ALERT;
        IUFindLight(motorLP, "RARunning")->s     = IPS_IDLE;
        IUFindLight(motorLP, "RAGoto")->s        = IPS_IDLE;
        IUFindLight(motorLP, "RAForward")->s     = IPS_IDLE;
        IUFindLight(motorLP, "RAHighspeed")->s   = IPS_IDLE;
    }
    else
    {
        IUFindLight(motorLP, "RAInitialized")->s = IPS_OK;
        IUFindLight(motorLP, "RARunning")->s     = (RARunning ? IPS_OK : IPS_BUSY);
        IUFindLight(motorLP, "RAGoto")->s        = ((RAStatus.slewmode == GOTO) ? IPS_OK : IPS_BUSY);
        IUFindLight(motorLP, "RAForward")->s     = ((RAStatus.direction == FORWARD) ? IPS_OK : IPS_BUSY);
        IUFindLight(motorLP, "RAHighspeed")->s   = ((RAStatus.speedmode == HIGHSPEED) ? IPS_OK : IPS_BUSY);
    }
}

void Skywatcher::GetDEMotorStatus(ILightVectorProperty *motorLP)
{
    ReadMotorStatus(Axis2);
    if (!DEInitialized)
    {
        IUFindLight(motorLP, "DEInitialized")->s = IPS_ALERT;
        IUFindLight(motorLP, "DERunning")->s     = IPS_IDLE;
        IUFindLight(motorLP, "DEGoto")->s        = IPS_IDLE;
        IUFindLight(motorLP, "DEForward")->s     = IPS_IDLE;
        IUFindLight(motorLP, "DEHighspeed")->s   = IPS_IDLE;
    }
    else
    {
        IUFindLight(motorLP, "DEInitialized")->s = IPS_OK;
        IUFindLight(motorLP, "DERunning")->s     = (DERunning ? IPS_OK : IPS_BUSY);
        IUFindLight(motorLP, "DEGoto")->s        = ((DEStatus.slewmode == GOTO) ? IPS_OK : IPS_BUSY);
        IUFindLight(motorLP, "DEForward")->s     = ((DEStatus.direction == FORWARD) ? IPS_OK : IPS_BUSY);
        IUFindLight(motorLP, "DEHighspeed")->s   = ((DEStatus.speedmode == HIGHSPEED) ? IPS_OK : IPS_BUSY);
    }
}

void Skywatcher::Init()
{
    wasinitialized = false;
    ReadMotorStatus(Axis1);
    ReadMotorStatus(Axis2);

    if (!RAInitialized && !DEInitialized)
    {
        //Read initial stepper values
        dispatch_command(GetAxisPosition, Axis1, NULL);
        read_eqmod();
        RAStepInit = Revu24str2long(response + 1);
        dispatch_command(GetAxisPosition, Axis2, NULL);
        read_eqmod();
        DEStepInit = Revu24str2long(response + 1);
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Motors not initialized -- read Init steps RAInit=%ld DEInit = %ld",
               __FUNCTION__, RAStepInit, DEStepInit);
        // Energize motors
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Powering motors", __FUNCTION__);
        dispatch_command(Initialize, Axis1, NULL);
        read_eqmod();
        dispatch_command(Initialize, Axis2, NULL);
        read_eqmod();
        RAStepHome = RAStepInit;
        DEStepHome = DEStepInit + (DESteps360 / 4);
    }
    else
    { // Mount already initialized by another driver / driver instance
        // use default configuration && leave unchanged encoder values
        wasinitialized = true;
        RAStepInit     = 0x800000;
        DEStepInit     = 0x800000;
        RAStepHome     = RAStepInit;
        DEStepHome     = DEStepInit + (DESteps360 / 4);
        DEBUGF(INDI::Logger::DBG_WARNING, "%s() : Motors already initialized", __FUNCTION__);
        DEBUGF(INDI::Logger::DBG_WARNING, "%s() : Setting default Init steps --  RAInit=%ld DEInit = %ld", __FUNCTION__,
               RAStepInit, DEStepInit);
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Setting Home steps RAHome=%ld DEHome = %ld", __FUNCTION__, RAStepHome,
           DEStepHome);

    if (not(reconnect))
    {
        reconnect = true;
        DEBUGF(INDI::Logger::DBG_WARNING, "%s() : First Initialization for this driver instance", __FUNCTION__);
        // Initialize unreadable mount feature
        //SetST4RAGuideRate('2');
        //SetST4DEGuideRate('2');
        //DEBUGF(INDI::Logger::DBG_WARNING, "%s() : Setting both ST4 guide rates to  0.5x (2)", __FUNCTION__);
    }

    // Problem with buildSkeleton: props are lost between connection/reconnections
    // should reset unreadable mount feature
    SetST4RAGuideRate('2');
    SetST4DEGuideRate('2');
    DEBUGF(INDI::Logger::DBG_WARNING, "%s() : Setting both ST4 guide rates to  0.5x (2)", __FUNCTION__);

    //Park status
    if (telescope->InitPark() == false)
    {
        telescope->SetAxis1Park(RAStepHome);
        telescope->SetAxis1ParkDefault(RAStepHome);

        telescope->SetAxis2Park(DEStepHome);
        telescope->SetAxis2ParkDefault(DEStepHome);
    }
    else
    {
        telescope->SetAxis1ParkDefault(RAStepHome);
        telescope->SetAxis2ParkDefault(DEStepHome);
    }

    if (telescope->isParked())
    {
        //TODO get Park position, set corresponding encoder values, mark mount as parked
        //parkSP->sp[0].s==ISS_ON
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Mount was parked", __FUNCTION__);
        //if (wasinitialized) {
        //  DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : leaving encoders unchanged",
        //	     __FUNCTION__);
        //} else {
        char cmdarg[7];
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Mount in Park position -- setting encoders RA=%ld DE = %ld",
               __FUNCTION__, (long)telescope->GetAxis1Park(), (long)telescope->GetAxis2Park());
        cmdarg[6] = '\0';
        long2Revu24str(telescope->GetAxis1Park(), cmdarg);
        dispatch_command(SetAxisPositionCmd, Axis1, cmdarg);
        read_eqmod();
        cmdarg[6] = '\0';
        long2Revu24str(telescope->GetAxis2Park(), cmdarg);
        dispatch_command(SetAxisPositionCmd, Axis2, cmdarg);
        read_eqmod();
        //}
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Mount was not parked", __FUNCTION__);
        if (wasinitialized)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : leaving encoders unchanged", __FUNCTION__);
        }
        else
        {
            //mount is supposed to be in the home position (pointing Celestial Pole)
            char cmdarg[7];
            DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : Mount in Home position -- setting encoders RA=%ld DE = %ld",
                   __FUNCTION__, RAStepHome, DEStepHome);
            cmdarg[6] = '\0';
            long2Revu24str(DEStepHome, cmdarg);
            dispatch_command(SetAxisPositionCmd, Axis2, cmdarg);
            read_eqmod();
            //DEBUGF(INDI::Logger::DBG_WARNING, "%s() : Mount is supposed to point North/South Celestial Pole", __FUNCTION__);
            //TODO mark mount as unparked?
        }
    }
}

void Skywatcher::InquireBoardVersion(ITextVectorProperty *boardTP)
{
    unsigned nprop             = 0;
    char *boardinfo[2];
    const char *boardinfopropnames[] = { "MOUNT_TYPE", "MOTOR_CONTROLLER" };

    /*
  unsigned long tmpMCVersion = 0;

  dispatch_command(InquireMotorBoardVersion, Axis1, NULL);
  read_eqmod();
  tmpMCVersion=Revu24str2long(response+1);
  MCVersion = ((tmpMCVersion & 0xFF) << 16) | ((tmpMCVersion & 0xFF00)) | ((tmpMCVersion & 0xFF0000) >> 16);
  MountCode=MCVersion & 0xFF;
  */
    minperiods[Axis1] = 6;
    minperiods[Axis2] = 6;
    nprop             = 2;
    //  strcpy(boardinfopropnames[0],"MOUNT_TYPE");
    boardinfo[0] = (char *)malloc(20 * sizeof(char));
    switch (MountCode)
    {
        case 0x00:
            strcpy(boardinfo[0], "EQ6");
            break;
        case 0x01:
            strcpy(boardinfo[0], "HEQ5");
            break;
        case 0x02:
            strcpy(boardinfo[0], "EQ5");
            break;
        case 0x03:
            strcpy(boardinfo[0], "EQ3");
            break;
        case 0x04:
            strcpy(boardinfo[0], "EQ8");
            break;
        case 0x05:
            strcpy(boardinfo[0], "AZEQ6");
            break;
        case 0x06:
            strcpy(boardinfo[0], "AZEQ5");
            break;
        case 0x80:
            strcpy(boardinfo[0], "GT");
            break;
        case 0x81:
            strcpy(boardinfo[0], "MF");
            break;
        case 0x82:
            strcpy(boardinfo[0], "114GT");
            break;
        case 0x90:
            strcpy(boardinfo[0], "DOB");
            break;
        case 0xF0:
            strcpy(boardinfo[0], "GEEHALEL");
            minperiods[Axis1] = 13;
            minperiods[Axis2] = 16;
            break;
        default:
            strcpy(boardinfo[0], "CUSTOM");
            break;
    }

    boardinfo[1] = (char *)malloc(5);
    sprintf(boardinfo[1], "%04lx", (MCVersion >> 8));
    boardinfo[1][4] = '\0';
    // should test this is ok
    IUUpdateText(boardTP, boardinfo, (char **)boardinfopropnames, nprop);
    IDSetText(boardTP, NULL);
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s(): MountCode = %d, MCVersion = %lx, setting minperiods Axis1=%d Axis2=%d",
           __FUNCTION__, MountCode, MCVersion, minperiods[Axis1], minperiods[Axis2]);
    /* Check supported mounts here */
    /*if ((MountCode == 0x80) || (MountCode == 0x81) || (MountCode == 0x82) || (MountCode == 0x90)) {

    throw EQModError(EQModError::ErrDisconnect, "Mount not supported %s (mount code %d)",
		     boardinfo[0], MountCode);
  }
  */
    free(boardinfo[0]);
    free(boardinfo[1]);
}

void Skywatcher::InquireFeatures()
{
    unsigned long rafeatures = 0, defeatures = 0;
    try
    {
        GetFeature(Axis1, GET_FEATURES_CMD);
        rafeatures = Revu24str2long(response + 1);
        GetFeature(Axis2, GET_FEATURES_CMD);
        defeatures = Revu24str2long(response + 1);
    }
    catch (EQModError e)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s(): Mount does not support query features  (%c command)", __FUNCTION__,
               GetFeatureCmd);
        rafeatures = 0;
        defeatures = 0;
    }
    if ((rafeatures & 0x000000F0) != (defeatures & 0x000000F0))
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s(): Found different features for RA (%d) and DEC (%d)", __FUNCTION__,
               rafeatures, defeatures);
    }
    if (rafeatures & 0x00000010)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s(): Found RA PPEC training on", __FUNCTION__);
    }
    if (defeatures & 0x00000010)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s(): Found DE PPEC training on", __FUNCTION__);
    }
    AxisFeatures[Axis1].inPPECTraining         = rafeatures & 0x00000010;
    AxisFeatures[Axis1].inPPEC                 = rafeatures & 0x00000020;
    AxisFeatures[Axis1].hasEncoder             = rafeatures & 0x00000001;
    AxisFeatures[Axis1].hasPPEC                = rafeatures & 0x00000002;
    AxisFeatures[Axis1].hasHomeIndexer         = rafeatures & 0x00000004;
    AxisFeatures[Axis1].isAZEQ                 = rafeatures & 0x00000008;
    AxisFeatures[Axis1].hasPolarLed            = rafeatures & 0x00001000;
    AxisFeatures[Axis1].hasCommonSlewStart     = rafeatures & 0x00002000; // supports :J3
    AxisFeatures[Axis1].hasHalfCurrentTracking = rafeatures & 0x00004000;
    AxisFeatures[Axis1].hasWifi                = rafeatures & 0x00008000;
    AxisFeatures[Axis2].inPPECTraining         = defeatures & 0x00000010;
    AxisFeatures[Axis2].inPPEC                 = defeatures & 0x00000020;
    AxisFeatures[Axis2].hasEncoder             = defeatures & 0x00000001;
    AxisFeatures[Axis2].hasPPEC                = defeatures & 0x00000002;
    AxisFeatures[Axis2].hasHomeIndexer         = defeatures & 0x00000004;
    AxisFeatures[Axis2].isAZEQ                 = defeatures & 0x00000008;
    AxisFeatures[Axis2].hasPolarLed            = defeatures & 0x00001000;
    AxisFeatures[Axis2].hasCommonSlewStart     = defeatures & 0x00002000; // supports :J3
    AxisFeatures[Axis2].hasHalfCurrentTracking = defeatures & 0x00004000;
    AxisFeatures[Axis2].hasWifi                = defeatures & 0x00008000;
}

bool Skywatcher::HasHomeIndexers()
{
    return (AxisFeatures[Axis1].hasHomeIndexer) && (AxisFeatures[Axis2].hasHomeIndexer);
}

bool Skywatcher::HasAuxEncoders()
{
    return (AxisFeatures[Axis1].hasEncoder) && (AxisFeatures[Axis2].hasEncoder);
}

bool Skywatcher::HasPPEC()
{
    return (AxisFeatures[Axis1].hasPPEC) && (AxisFeatures[Axis2].hasPPEC);
}

void Skywatcher::InquireRAEncoderInfo(INumberVectorProperty *encoderNP)
{
    double steppersvalues[3];
    const char *steppersnames[] = { "RASteps360", "RAStepsWorm", "RAHighspeedRatio" };
    // Steps per 360 degrees
    dispatch_command(InquireGridPerRevolution, Axis1, NULL);
    read_eqmod();
    RASteps360        = Revu24str2long(response + 1);
    steppersvalues[0] = (double)RASteps360;

    // Steps per Worm
    dispatch_command(InquireTimerInterruptFreq, Axis1, NULL);
    read_eqmod();
    RAStepsWorm = Revu24str2long(response + 1);
    // There is a bug in the earlier version firmware(Before 2.00) of motor controller MC001.
    // Overwrite the GearRatio reported by the MC for 80GT mount and 114GT mount.
    if ((MCVersion & 0x0000FF) == 0x80)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s: forcing RAStepsWorm for 80GT Mount (%x in place of %x)", __FUNCTION__,
               0x162B97, RAStepsWorm);
        RAStepsWorm = 0x162B97; // for 80GT mount
    }
    if ((MCVersion & 0x0000FF) == 0x82)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s: forcing RAStepsWorm for 114GT Mount (%x in place of %x)", __FUNCTION__,
               0x205318, RAStepsWorm);
        RAStepsWorm = 0x205318; // for 114GT mount
    }
    steppersvalues[1] = (double)RAStepsWorm;

    // Highspeed Ratio
    dispatch_command(InquireHighSpeedRatio, Axis1, NULL);
    read_eqmod();
    //RAHighspeedRatio=Revu24str2long(response+1);
    RAHighspeedRatio = Highstr2long(response + 1);

    steppersvalues[2] = (double)RAHighspeedRatio;
    // should test this is ok
    IUUpdateNumber(encoderNP, steppersvalues, (char **)steppersnames, 3);
    IDSetNumber(encoderNP, NULL);

    backlashperiod[Axis1] =
        (long)(((SKYWATCHER_STELLAR_DAY * (double)RAStepsWorm) / (double)RASteps360) / SKYWATCHER_BACKLASH_SPEED_RA);
}

void Skywatcher::InquireDEEncoderInfo(INumberVectorProperty *encoderNP)
{
    double steppersvalues[3];
    const char *steppersnames[] = { "DESteps360", "DEStepsWorm", "DEHighspeedRatio" };
    // Steps per 360 degrees
    dispatch_command(InquireGridPerRevolution, Axis2, NULL);
    read_eqmod();
    DESteps360        = Revu24str2long(response + 1);
    steppersvalues[0] = (double)DESteps360;

    // Steps per Worm
    dispatch_command(InquireTimerInterruptFreq, Axis2, NULL);
    read_eqmod();
    DEStepsWorm = Revu24str2long(response + 1);
    // There is a bug in the earlier version firmware(Before 2.00) of motor controller MC001.
    // Overwrite the GearRatio reported by the MC for 80GT mount and 114GT mount.
    if ((MCVersion & 0x0000FF) == 0x80)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s: forcing DEStepsWorm for 80GT Mount (%x in place of %x)", __FUNCTION__,
               0x162B97, DEStepsWorm);
        DEStepsWorm = 0x162B97; // for 80GT mount
    }
    if ((MCVersion & 0x0000FF) == 0x82)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s: forcing DEStepsWorm for 114GT Mount (%x in place of %x)", __FUNCTION__,
               0x205318, DEStepsWorm);
        DEStepsWorm = 0x205318; // for 114GT mount
    }

    steppersvalues[1] = (double)DEStepsWorm;

    // Highspeed Ratio
    dispatch_command(InquireHighSpeedRatio, Axis2, NULL);
    read_eqmod();
    //DEHighspeedRatio=Revu24str2long(response+1);
    DEHighspeedRatio  = Highstr2long(response + 1);
    steppersvalues[2] = (double)DEHighspeedRatio;
    // should test this is ok
    IUUpdateNumber(encoderNP, steppersvalues, (char **)steppersnames, 3);
    IDSetNumber(encoderNP, NULL);
    backlashperiod[Axis2] =
        (long)(((SKYWATCHER_STELLAR_DAY * (double)DEStepsWorm) / (double)DESteps360) / SKYWATCHER_BACKLASH_SPEED_DE);
}

bool Skywatcher::IsRARunning()
{
    CheckMotorStatus(Axis1);
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %s", __FUNCTION__, (RARunning ? "true" : "false"));
    return (RARunning);
}

bool Skywatcher::IsDERunning()
{
    CheckMotorStatus(Axis2);
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() = %s", __FUNCTION__, (DERunning ? "true" : "false"));
    return (DERunning);
}

void Skywatcher::ReadMotorStatus(SkywatcherAxis axis)
{
    dispatch_command(GetAxisStatus, axis, NULL);
    read_eqmod();
    switch (axis)
    {
        case Axis1:
            RAInitialized = (response[3] & 0x01);
            RARunning     = (response[2] & 0x01);
            if (response[1] & 0x01)
                RAStatus.slewmode = SLEW;
            else
                RAStatus.slewmode = GOTO;
            if (response[1] & 0x02)
                RAStatus.direction = BACKWARD;
            else
                RAStatus.direction = FORWARD;
            if (response[1] & 0x04)
                RAStatus.speedmode = HIGHSPEED;
            else
                RAStatus.speedmode = LOWSPEED;
            break;
        case Axis2:
            DEInitialized = (response[3] & 0x01);
            DERunning     = (response[2] & 0x01);
            if (response[1] & 0x01)
                DEStatus.slewmode = SLEW;
            else
                DEStatus.slewmode = GOTO;
            if (response[1] & 0x02)
                DEStatus.direction = BACKWARD;
            else
                DEStatus.direction = FORWARD;
            if (response[1] & 0x04)
                DEStatus.speedmode = HIGHSPEED;
            else
                DEStatus.speedmode = LOWSPEED;
        default:;
    }
    gettimeofday(&lastreadmotorstatus[axis], NULL);
}

void Skywatcher::SlewRA(double rate)
{
    double absrate       = fabs(rate);
    unsigned long period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : rate = %g", __FUNCTION__, rate);

    if (RARunning && (RAStatus.slewmode == GOTO))
    {
        throw EQModError(EQModError::ErrInvalidCmd, "Can not slew while goto is in progress");
    }

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        throw EQModError(EQModError::ErrInvalidParameter,
                         "Speed rate out of limits: %.2fx Sidereal (min=%.2f, max=%.2f)", absrate, MIN_RATE, MAX_RATE);
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / RAHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period = (long)(((SKYWATCHER_STELLAR_DAY * (double)RAStepsWorm) / (double)RASteps360) / absrate);
    if (rate >= 0.0)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    SetMotion(Axis1, newstatus);
    SetSpeed(Axis1, period);
    if (!RARunning)
        StartMotor(Axis1);
}

void Skywatcher::SlewDE(double rate)
{
    double absrate       = fabs(rate);
    unsigned long period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : rate = %g", __FUNCTION__, rate);

    if (DERunning && (DEStatus.slewmode == GOTO))
    {
        throw EQModError(EQModError::ErrInvalidCmd, "Can not slew while goto is in progress");
    }

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        throw EQModError(EQModError::ErrInvalidParameter,
                         "Speed rate out of limits: %.2fx Sidereal (min=%.2f, max=%.2f)", absrate, MIN_RATE, MAX_RATE);
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / DEHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period = (long)(((SKYWATCHER_STELLAR_DAY * (double)DEStepsWorm) / (double)DESteps360) / absrate);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Slewing DE at %.2f %.2f %x %f\n", rate, absrate, period,
           (((SKYWATCHER_STELLAR_DAY * (double)RAStepsWorm) / (double)RASteps360) / absrate));

    if (rate >= 0.0)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    SetMotion(Axis2, newstatus);
    SetSpeed(Axis2, period);
    if (!DERunning)
        StartMotor(Axis2);
}

void Skywatcher::SlewTo(long deltaraencoder, long deltadeencoder)
{
    SkywatcherAxisStatus newstatus;
    bool useHighSpeed        = false;
    unsigned long lowperiod = 18, lowspeedmargin = 20000, breaks = 400;
    /* highperiod = RA 450X DE (+5) 200x, low period 32x */

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : deltaRA = %ld deltaDE = %ld", __FUNCTION__, deltaraencoder, deltadeencoder);

    newstatus.slewmode = GOTO;
    if (deltaraencoder >= 0)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    if (deltaraencoder < 0)
        deltaraencoder = -deltaraencoder;
    if (deltaraencoder > (long)lowspeedmargin)
        useHighSpeed = true;
    else
        useHighSpeed = false;
    if (useHighSpeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (deltaraencoder > 0)
    {
        SetMotion(Axis1, newstatus);
        if (useHighSpeed)
            SetSpeed(Axis1, minperiods[Axis1]);
        else
            SetSpeed(Axis1, lowperiod);
        SetTarget(Axis1, deltaraencoder);
        if (useHighSpeed)
            breaks = ((deltaraencoder > 3200) ? 3200 : deltaraencoder / 10);
        else
            breaks = ((deltaraencoder > 200) ? 200 : deltaraencoder / 10);
        SetTargetBreaks(Axis1, breaks);
        StartMotor(Axis1);
    }

    if (deltadeencoder >= 0)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    if (deltadeencoder < 0)
        deltadeencoder = -deltadeencoder;
    if (deltadeencoder > (long)lowspeedmargin)
        useHighSpeed = true;
    else
        useHighSpeed = false;
    if (useHighSpeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (deltadeencoder > 0)
    {
        SetMotion(Axis2, newstatus);
        if (useHighSpeed)
            SetSpeed(Axis2, minperiods[Axis2]);
        else
            SetSpeed(Axis2, lowperiod);
        SetTarget(Axis2, deltadeencoder);
        if (useHighSpeed)
            breaks = ((deltadeencoder > 3200) ? 3200 : deltadeencoder / 10);
        else
            breaks = ((deltadeencoder > 200) ? 200 : deltadeencoder / 10);
        SetTargetBreaks(Axis2, breaks);
        StartMotor(Axis2);
    }
}

void Skywatcher::AbsSlewTo(unsigned long raencoder, unsigned long deencoder, bool raup, bool deup)
{
    SkywatcherAxisStatus newstatus;
    bool useHighSpeed = false;
    long deltaraencoder, deltadeencoder;
    unsigned long lowperiod = 18, lowspeedmargin = 20000, breaks = 400;
    /* highperiod = RA 450X DE (+5) 200x, low period 32x */

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : absRA = %ld raup = %c absDE = %ld deup = %c", __FUNCTION__, raencoder,
           (raup ? '1' : '0'), deencoder, (deup ? '1' : '0'));

    deltaraencoder = raencoder - RAStep;
    deltadeencoder = deencoder - DEStep;

    newstatus.slewmode = GOTO;
    if (raup)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    if (deltaraencoder < 0)
        deltaraencoder = -deltaraencoder;
    if (deltaraencoder > (long)lowspeedmargin)
        useHighSpeed = true;
    else
        useHighSpeed = false;
    if (useHighSpeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (deltaraencoder > 0)
    {
        SetMotion(Axis1, newstatus);
        if (useHighSpeed)
            SetSpeed(Axis1, minperiods[Axis1]);
        else
            SetSpeed(Axis1, lowperiod);
        SetAbsTarget(Axis1, raencoder);
        if (useHighSpeed)
            breaks = ((deltaraencoder > 3200) ? 3200 : deltaraencoder / 10);
        else
            breaks = ((deltaraencoder > 200) ? 200 : deltaraencoder / 10);
        breaks = (raup ? (raencoder - breaks) : (raencoder + breaks));
        SetAbsTargetBreaks(Axis1, breaks);
        StartMotor(Axis1);
    }

    if (deup)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    if (deltadeencoder < 0)
        deltadeencoder = -deltadeencoder;
    if (deltadeencoder > (long)lowspeedmargin)
        useHighSpeed = true;
    else
        useHighSpeed = false;
    if (useHighSpeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (deltadeencoder > 0)
    {
        SetMotion(Axis2, newstatus);
        if (useHighSpeed)
            SetSpeed(Axis2, minperiods[Axis2]);
        else
            SetSpeed(Axis2, lowperiod);
        SetAbsTarget(Axis2, deencoder);
        if (useHighSpeed)
            breaks = ((deltadeencoder > 3200) ? 3200 : deltadeencoder / 10);
        else
            breaks = ((deltadeencoder > 200) ? 200 : deltadeencoder / 10);
        breaks = (deup ? (deencoder - breaks) : (deencoder + breaks));
        SetAbsTargetBreaks(Axis2, breaks);
        StartMotor(Axis2);
    }
}

void Skywatcher::SetRARate(double rate)
{
    double absrate       = fabs(rate);
    unsigned long period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : rate = %g", __FUNCTION__, rate);

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        throw EQModError(EQModError::ErrInvalidParameter,
                         "Speed rate out of limits: %.2fx Sidereal (min=%.2f, max=%.2f)", absrate, MIN_RATE, MAX_RATE);
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / RAHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period              = (long)(((SKYWATCHER_STELLAR_DAY * (double)RAStepsWorm) / (double)RASteps360) / absrate);
    newstatus.direction = ((rate >= 0.0) ? FORWARD : BACKWARD);
    //newstatus.slewmode=RAStatus.slewmode;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (RARunning)
    {
        if (newstatus.speedmode != RAStatus.speedmode)
            throw EQModError(EQModError::ErrInvalidParameter,
                             "Can not change rate while motor is running (speedmode differs).");
        if (newstatus.direction != RAStatus.direction)
            throw EQModError(EQModError::ErrInvalidParameter,
                             "Can not change rate while motor is running (direction differs).");
    }
    SetMotion(Axis1, newstatus);
    SetSpeed(Axis1, period);
}

void Skywatcher::SetDERate(double rate)
{
    double absrate       = fabs(rate);
    unsigned long period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : rate = %g", __FUNCTION__, rate);

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        throw EQModError(EQModError::ErrInvalidParameter,
                         "Speed rate out of limits: %.2fx Sidereal (min=%.2f, max=%.2f)", absrate, MIN_RATE, MAX_RATE);
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / DEHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period              = (long)(((SKYWATCHER_STELLAR_DAY * (double)DEStepsWorm) / (double)DESteps360) / absrate);
    newstatus.direction = ((rate >= 0.0) ? FORWARD : BACKWARD);
    //newstatus.slewmode=DEStatus.slewmode;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (DERunning)
    {
        if (newstatus.speedmode != DEStatus.speedmode)
            throw EQModError(EQModError::ErrInvalidParameter,
                             "Can not change rate while motor is running (speedmode differs).");
        if (newstatus.direction != DEStatus.direction)
            throw EQModError(EQModError::ErrInvalidParameter,
                             "Can not change rate while motor is running (direction differs).");
    }
    SetMotion(Axis2, newstatus);
    SetSpeed(Axis2, period);
}

void Skywatcher::StartRATracking(double trackspeed)
{
    double rate;
    if (trackspeed != 0.0)
        rate = trackspeed / SKYWATCHER_STELLAR_SPEED;
    else
        rate = 0.0;
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : trackspeed = %g arcsecs/s, computed rate = %g", __FUNCTION__, trackspeed,
           rate);
    if (rate != 0.0)
    {
        SetRARate(rate);
        if (!RARunning)
            StartMotor(Axis1);
    }
    else
        StopMotor(Axis1);
}

void Skywatcher::StartDETracking(double trackspeed)
{
    double rate;
    if (trackspeed != 0.0)
        rate = trackspeed / SKYWATCHER_STELLAR_SPEED;
    else
        rate = 0.0;
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : trackspeed = %g arcsecs/s, computed rate = %g", __FUNCTION__, trackspeed,
           rate);
    if (rate != 0.0)
    {
        SetDERate(rate);
        if (!DERunning)
            StartMotor(Axis2);
    }
    else
        StopMotor(Axis2);
}

void Skywatcher::SetSpeed(SkywatcherAxis axis, unsigned long period)
{
    char cmd[7];
    SkywatcherAxisStatus *currentstatus;

    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- period=%ld", __FUNCTION__, AxisCmd[axis], period);

    ReadMotorStatus(axis);
    if (axis == Axis1)
        currentstatus = &RAStatus;
    else
        currentstatus = &DEStatus;
    if ((currentstatus->speedmode == HIGHSPEED) && (period < minperiods[axis]))
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "Setting axis %c period to minimum. Requested is %d, minimum is %d\n", AxisCmd[axis],
               period, minperiods[axis]);
        period = minperiods[axis];
    }
    long2Revu24str(period, cmd);

    if ((axis == Axis1) && (RARunning && (currentstatus->slewmode == GOTO || currentstatus->speedmode == HIGHSPEED)))
        throw EQModError(EQModError::ErrInvalidParameter,
                         "Can not change speed while motor is running and in goto or highspeed slew.");
    if ((axis == Axis2) && (DERunning && (currentstatus->slewmode == GOTO || currentstatus->speedmode == HIGHSPEED)))
        throw EQModError(EQModError::ErrInvalidParameter,
                         "Can not change speed while motor is running and in goto or highspeed slew.");

    if (axis == Axis1)
        RAPeriod = period;
    else
        DEPeriod = period;
    dispatch_command(SetStepPeriod, axis, cmd);
    read_eqmod();
}

void Skywatcher::SetTarget(SkywatcherAxis axis, unsigned long increment)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- increment=%ld", __FUNCTION__, AxisCmd[axis], increment);
    long2Revu24str(increment, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetGotoTargetIncrement, axis, cmd);
    read_eqmod();
    Target[axis] = increment;
}

void Skywatcher::SetTargetBreaks(SkywatcherAxis axis, unsigned long increment)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- increment=%ld", __FUNCTION__, AxisCmd[axis], increment);
    long2Revu24str(increment, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetBreakPointIncrement, axis, cmd);
    read_eqmod();
    TargetBreaks[axis] = increment;
}

void Skywatcher::SetAbsTarget(SkywatcherAxis axis, unsigned long target)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- target=%ld", __FUNCTION__, AxisCmd[axis], target);
    long2Revu24str(target, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetGotoTarget, axis, cmd);
    read_eqmod();
    Target[axis] = target;
}

void Skywatcher::SetAbsTargetBreaks(SkywatcherAxis axis, unsigned long breakstep)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- breakstep=%ld", __FUNCTION__, AxisCmd[axis], breakstep);
    long2Revu24str(breakstep, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetBreakStep, axis, cmd);
    read_eqmod();
    TargetBreaks[axis] = breakstep;
}

void Skywatcher::SetFeature(SkywatcherAxis axis, unsigned long command)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- command=%ld", __FUNCTION__, AxisCmd[axis], command);
    long2Revu24str(command, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetFeatureCmd, axis, cmd);
    read_eqmod();
}

void Skywatcher::GetFeature(SkywatcherAxis axis, unsigned long command)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- command=%ld", __FUNCTION__, AxisCmd[axis], command);
    long2Revu24str(command, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(GetFeatureCmd, axis, cmd);
    read_eqmod();
}

void Skywatcher::GetIndexer(SkywatcherAxis axis)
{
    GetFeature(axis, GET_INDEXER_CMD);
    lastreadIndexer[axis] = Revu24str2long(response + 1);
}

void Skywatcher::GetRAIndexer()
{
    GetIndexer(Axis1);
}

void Skywatcher::GetDEIndexer()
{
    GetIndexer(Axis2);
}

void Skywatcher::ResetIndexer(SkywatcherAxis axis)
{
    SetFeature(axis, RESET_HOME_INDEXER_CMD);
}

void Skywatcher::ResetRAIndexer()
{
    ResetIndexer(Axis1);
}

void Skywatcher::ResetDEIndexer()
{
    ResetIndexer(Axis2);
}

void Skywatcher::TurnEncoder(SkywatcherAxis axis, bool on)
{
    unsigned long command;
    if (on)
        command = ENCODER_ON_CMD;
    else
        command = ENCODER_OFF_CMD;
    SetFeature(axis, command);
}

void Skywatcher::TurnRAEncoder(bool on)
{
    TurnEncoder(Axis1, on);
}

void Skywatcher::TurnDEEncoder(bool on)
{
    TurnEncoder(Axis2, on);
}

unsigned long Skywatcher::ReadEncoder(SkywatcherAxis axis)
{
    dispatch_command(InquireAuxEncoder, axis, NULL);
    read_eqmod();
    return Revu24str2long(response + 1);
}

unsigned long Skywatcher::GetRAAuxEncoder()
{
    return ReadEncoder(Axis1);
}

unsigned long Skywatcher::GetDEAuxEncoder()
{
    return ReadEncoder(Axis2);
}

void Skywatcher::SetST4RAGuideRate(unsigned char r)
{
    SetST4GuideRate(Axis1, r);
}

void Skywatcher::SetST4DEGuideRate(unsigned char r)
{
    SetST4GuideRate(Axis2, r);
}

void Skywatcher::SetST4GuideRate(SkywatcherAxis axis, unsigned char r)
{
    char cmd[2];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- rate=%c", __FUNCTION__, AxisCmd[axis], r);
    cmd[0] = r;
    cmd[1] = '\0';
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetST4GuideRateCmd, axis, cmd);
    read_eqmod();
}

void Skywatcher::TurnPPECTraining(SkywatcherAxis axis, bool on)
{
    unsigned long command;
    if (on)
        command = START_PPEC_TRAINING_CMD;
    else
        command = STOP_PPEC_TRAINING_CMD;
    SetFeature(axis, command);
}

void Skywatcher::TurnRAPPECTraining(bool on)
{
    TurnPPECTraining(Axis1, on);
}
void Skywatcher::TurnDEPPECTraining(bool on)
{
    TurnPPECTraining(Axis2, on);
}

void Skywatcher::TurnPPEC(SkywatcherAxis axis, bool on)
{
    unsigned long command;
    if (on)
        command = TURN_PPEC_ON_CMD;
    else
        command = TURN_PPEC_OFF_CMD;
    SetFeature(axis, command);
}

void Skywatcher::TurnRAPPEC(bool on)
{
    TurnPPEC(Axis1, on);
}
void Skywatcher::TurnDEPPEC(bool on)
{
    TurnPPEC(Axis2, on);
}

void Skywatcher::GetPPECStatus(SkywatcherAxis axis, bool *intraining, bool *inppec)
{
    unsigned long features = 0;
    GetFeature(axis, GET_FEATURES_CMD);
    features    = Revu24str2long(response + 1);
    *intraining = AxisFeatures[axis].inPPECTraining = features & 0x00000010;
    *inppec = AxisFeatures[axis].inPPEC = features & 0x00000020;
}
void Skywatcher::GetRAPPECStatus(bool *intraining, bool *inppec)
{
    return GetPPECStatus(Axis1, intraining, inppec);
}

void Skywatcher::GetDEPPECStatus(bool *intraining, bool *inppec)
{
    return GetPPECStatus(Axis2, intraining, inppec);
}

void Skywatcher::SetAxisPosition(SkywatcherAxis axis, unsigned long step)
{
    char cmd[7];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- step=%ld", __FUNCTION__, AxisCmd[axis], step);
    long2Revu24str(step, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetAxisPositionCmd, axis, cmd);
    read_eqmod();
}

void Skywatcher::SetRAAxisPosition(unsigned long step)
{
    SetAxisPosition(Axis1, step);
}

void Skywatcher::SetDEAxisPosition(unsigned long step)
{
    SetAxisPosition(Axis2, step);
}

void Skywatcher::StartMotor(SkywatcherAxis axis)
{
    bool usebacklash       = UseBacklash[axis];
    unsigned long backlash = Backlash[axis];
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c", __FUNCTION__, AxisCmd[axis]);

    if (usebacklash)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Checking backlash compensation for axis %c", AxisCmd[axis]);
        if (NewStatus[axis].direction != LastRunningStatus[axis].direction)
        {
            unsigned long currentsteps;
            char cmd[7];
            char motioncmd[3] = "20";                                               // lowspeed goto
            motioncmd[1]      = (NewStatus[axis].direction == FORWARD ? '0' : '1'); // same direction
            bool *motorrunning;
            struct timespec wait;
            DEBUGF(INDI::Logger::DBG_SESSION, "Performing backlash compensation for axis %c, microsteps = %d", AxisCmd[axis],
                   backlash);
            // Axis Position
            dispatch_command(GetAxisPosition, axis, NULL);
            read_eqmod();
            currentsteps = Revu24str2long(response + 1);
            // Backlash Speed
            long2Revu24str(backlashperiod[axis], cmd);
            dispatch_command(SetStepPeriod, axis, cmd);
            read_eqmod();
            // Backlash motion mode
            dispatch_command(SetMotionMode, axis, motioncmd);
            read_eqmod();
            // Target for backlash
            long2Revu24str(backlash, cmd);
            dispatch_command(SetGotoTargetIncrement, axis, cmd);
            read_eqmod();
            // Target breaks for backlash (no break steps)
            long2Revu24str(backlash / 10, cmd);
            dispatch_command(SetBreakPointIncrement, axis, cmd);
            read_eqmod();
            // Start Backlash
            dispatch_command(StartMotion, axis, NULL);
            read_eqmod();
            // Wait end of backlash
            if (axis == Axis1)
                motorrunning = &RARunning;
            else
                motorrunning = &DERunning;
            wait.tv_sec  = 0;
            wait.tv_nsec = 100000000; // 100ms
            ReadMotorStatus(axis);
            while (*motorrunning)
            {
                nanosleep(&wait, NULL);
                ReadMotorStatus(axis);
            }
            // Restore microsteps
            long2Revu24str(currentsteps, cmd);
            dispatch_command(SetAxisPositionCmd, axis, cmd);
            read_eqmod();
            // Restore Speed
            long2Revu24str((axis == Axis1 ? RAPeriod : DEPeriod), cmd);
            dispatch_command(SetStepPeriod, axis, cmd);
            read_eqmod();
            // Restore motion mode
            switch (NewStatus[axis].slewmode)
            {
                case SLEW:
                    if (NewStatus[axis].speedmode == LOWSPEED)
                        motioncmd[0] = '1';
                    else
                        motioncmd[0] = '3';
                    break;
                case GOTO:
                    if (NewStatus[axis].speedmode == LOWSPEED)
                        motioncmd[0] = '2';
                    else
                        motioncmd[0] = '0';
                    break;
                default:
                    motioncmd[0] = '1';
                    break;
            }
            dispatch_command(SetMotionMode, axis, motioncmd);
            read_eqmod();
            // Restore Target
            long2Revu24str(Target[axis], cmd);
            dispatch_command(SetGotoTargetIncrement, axis, cmd);
            read_eqmod();
            // Restore Target breaks
            long2Revu24str(TargetBreaks[axis], cmd);
            dispatch_command(SetBreakPointIncrement, axis, cmd);
            read_eqmod();
        }
    }
    dispatch_command(StartMotion, axis, NULL);
    read_eqmod();
}

void Skywatcher::StopRA()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : calling RA StopWaitMotor", __FUNCTION__);
    StopWaitMotor(Axis1);
}

void Skywatcher::StopDE()
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s() : calling DE StopWaitMotor", __FUNCTION__);
    StopWaitMotor(Axis2);
}

void Skywatcher::SetMotion(SkywatcherAxis axis, SkywatcherAxisStatus newstatus)
{
    char motioncmd[3];
    SkywatcherAxisStatus *currentstatus;

    DEBUGF(DBG_MOUNT, "%s() : Axis = %c -- dir=%s mode=%s speedmode=%s", __FUNCTION__, AxisCmd[axis],
           ((newstatus.direction == FORWARD) ? "forward" : "backward"),
           ((newstatus.slewmode == SLEW) ? "slew" : "goto"),
           ((newstatus.speedmode == LOWSPEED) ? "lowspeed" : "highspeed"));

    CheckMotorStatus(axis);
    if (axis == Axis1)
        currentstatus = &RAStatus;
    else
        currentstatus = &DEStatus;
    motioncmd[2] = '\0';
    switch (newstatus.slewmode)
    {
        case SLEW:
            if (newstatus.speedmode == LOWSPEED)
                motioncmd[0] = '1';
            else
                motioncmd[0] = '3';
            break;
        case GOTO:
            if (newstatus.speedmode == LOWSPEED)
                motioncmd[0] = '2';
            else
                motioncmd[0] = '0';
            break;
        default:
            motioncmd[0] = '1';
            break;
    }
    if (newstatus.direction == FORWARD)
        motioncmd[1] = '0';
    else
        motioncmd[1] = '1';
    /*
#ifdef STOP_WHEN_MOTION_CHANGED
  StopWaitMotor(axis);
  dispatch_command(SetMotionMode, axis, motioncmd);
  read_eqmod();
#else
  */
    if ((newstatus.direction != currentstatus->direction) || (newstatus.speedmode != currentstatus->speedmode) ||
        (newstatus.slewmode != currentstatus->slewmode))
    {
        StopWaitMotor(axis);
        dispatch_command(SetMotionMode, axis, motioncmd);
        read_eqmod();
    }
    //#endif
    NewStatus[axis] = newstatus;
}

void Skywatcher::ResetMotions()
{
    char motioncmd[3];
    SkywatcherAxisStatus newstatus;

    DEBUGF(DBG_MOUNT, "%s() ", __FUNCTION__);

    motioncmd[2] = '\0';
    //set to SLEW/LOWSPEED
    newstatus.slewmode  = SLEW;
    newstatus.speedmode = LOWSPEED;
    motioncmd[0]        = '1';
    // Keep directions
    CheckMotorStatus(Axis1);
    newstatus.direction = RAStatus.direction;
    if (RAStatus.direction == FORWARD)
        motioncmd[1] = '0';
    else
        motioncmd[1] = '1';
    dispatch_command(SetMotionMode, Axis1, motioncmd);
    read_eqmod();
    NewStatus[Axis1] = newstatus;

    CheckMotorStatus(Axis2);
    newstatus.direction = DEStatus.direction;
    if (DEStatus.direction == FORWARD)
        motioncmd[1] = '0';
    else
        motioncmd[1] = '1';
    dispatch_command(SetMotionMode, Axis2, motioncmd);
    read_eqmod();
    NewStatus[Axis2] = newstatus;
}

void Skywatcher::StopMotor(SkywatcherAxis axis)
{
    ReadMotorStatus(axis);
    if (axis == Axis1 && RARunning)
        LastRunningStatus[Axis1] = RAStatus;
    if (axis == Axis2 && DERunning)
        LastRunningStatus[Axis2] = DEStatus;
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c", __FUNCTION__, AxisCmd[axis]);
    dispatch_command(NotInstantAxisStop, axis, NULL);
    read_eqmod();
}

void Skywatcher::InstantStopMotor(SkywatcherAxis axis)
{
    ReadMotorStatus(axis);
    if (axis == Axis1 && RARunning)
        LastRunningStatus[Axis1] = RAStatus;
    if (axis == Axis2 && DERunning)
        LastRunningStatus[Axis2] = DEStatus;
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c", __FUNCTION__, AxisCmd[axis]);
    dispatch_command(InstantAxisStop, axis, NULL);
    read_eqmod();
}

void Skywatcher::StopWaitMotor(SkywatcherAxis axis)
{
    bool *motorrunning;
    struct timespec wait;
    ReadMotorStatus(axis);
    if (axis == Axis1 && RARunning)
        LastRunningStatus[Axis1] = RAStatus;
    if (axis == Axis2 && DERunning)
        LastRunningStatus[Axis2] = DEStatus;
    DEBUGF(DBG_MOUNT, "%s() : Axis = %c", __FUNCTION__, AxisCmd[axis]);
    dispatch_command(NotInstantAxisStop, axis, NULL);
    read_eqmod();
    if (axis == Axis1)
        motorrunning = &RARunning;
    else
        motorrunning = &DERunning;
    wait.tv_sec  = 0;
    wait.tv_nsec = 100000000; // 100ms
    ReadMotorStatus(axis);
    while (*motorrunning)
    {
        nanosleep(&wait, NULL);
        ReadMotorStatus(axis);
    }
}

/* Utilities */

void Skywatcher::CheckMotorStatus(SkywatcherAxis axis)
{
    struct timeval now;
    DEBUGF(DBG_SCOPE_STATUS, "%s() : Axis = %c", __FUNCTION__, AxisCmd[axis]);
    gettimeofday(&now, NULL);
    if (((now.tv_sec - lastreadmotorstatus[axis].tv_sec) + ((now.tv_usec - lastreadmotorstatus[axis].tv_usec) / 1e6)) >
        SKYWATCHER_MAXREFRESH)
        ReadMotorStatus(axis);
}

double Skywatcher::get_min_rate()
{
    return MIN_RATE;
}

double Skywatcher::get_max_rate()
{
    return MAX_RATE;
}

bool Skywatcher::dispatch_command(SkywatcherCommand cmd, SkywatcherAxis axis, char *command_arg)
{
    int err_code = 0, nbytes_written = 0;

    // Clear string
    command[0] = '\0';

    if (command_arg == NULL)
        snprintf(command, SKYWATCHER_MAX_CMD, "%c%c%c%c", SkywatcherLeadingChar, cmd, AxisCmd[axis], SkywatcherTrailingChar);
    else
        snprintf(command, SKYWATCHER_MAX_CMD, "%c%c%c%s%c", SkywatcherLeadingChar, cmd, AxisCmd[axis], command_arg,
                 SkywatcherTrailingChar);
    if (!isSimulation())
    {
        tcflush(PortFD, TCIOFLUSH);

        if ((err_code = tty_write_string(PortFD, command, &nbytes_written) != TTY_OK))
        {
            char ttyerrormsg[ERROR_MSG_LENGTH];
            tty_error_msg(err_code, ttyerrormsg, ERROR_MSG_LENGTH);
            throw EQModError(EQModError::ErrDisconnect, "tty write failed, check connection: %s", ttyerrormsg);
            return false;
        }
    }
    else
    {
        telescope->simulator->receive_cmd(command, &nbytes_written);
    }

    //if (INDI::Logger::debugSerial(cmd)) {
    command[nbytes_written - 1] = '\0'; //hmmm, remove \r, the  SkywatcherTrailingChar
    DEBUGF(DBG_COMM, "dispatch_command: \"%s\", %d bytes written", command, nbytes_written);
    debugnextread = true;
    //}
    return true;
}

bool Skywatcher::read_eqmod()
{
    int err_code = 0, nbytes_read = 0;

    // Clear string
    response[0] = '\0';
    if (!isSimulation())
    {
        //Have to onsider cases when we read ! (error) or 0x01 (buffer overflow)
        // Read until encountring a CR
        if ((err_code = tty_read_section(PortFD, response, 0x0D, 15, &nbytes_read)) != TTY_OK)
        {
            char ttyerrormsg[ERROR_MSG_LENGTH];
            tty_error_msg(err_code, ttyerrormsg, ERROR_MSG_LENGTH);
            throw EQModError(EQModError::ErrDisconnect, "tty read failed, check connection: %s", ttyerrormsg);
            return false;
        }
    }
    else
    {
        telescope->simulator->send_reply(response, &nbytes_read);
    }
    // Remove CR
    response[nbytes_read - 1] = '\0';

    if (debugnextread)
    {
        DEBUGF(DBG_COMM, "read_eqmod: \"%s\", %d bytes read", response, nbytes_read);
        debugnextread = false;
    }
    switch (response[0])
    {
        case '=':
            break;
        case '!':
            throw EQModError(EQModError::ErrCmdFailed, "Failed command %s - Reply %s", command, response);
            return false;
        default:
            throw EQModError(EQModError::ErrInvalidCmd, "Invalid response to command %s - Reply %s", command, response);
            return false;
    }

    return true;
}

unsigned long Skywatcher::Revu24str2long(char *s)
{
    unsigned long res = 0;
    res               = HEX(s[4]);
    res <<= 4;
    res |= HEX(s[5]);
    res <<= 4;
    res |= HEX(s[2]);
    res <<= 4;
    res |= HEX(s[3]);
    res <<= 4;
    res |= HEX(s[0]);
    res <<= 4;
    res |= HEX(s[1]);
    return res;
}

unsigned long Skywatcher::Highstr2long(char *s)
{
    unsigned long res = 0;
    res               = HEX(s[0]);
    res <<= 4;
    res |= HEX(s[1]);
    return res;
}

void Skywatcher::long2Revu24str(unsigned long n, char *str)
{
    char hexa[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    str[0]        = hexa[(n & 0xF0) >> 4];
    str[1]        = hexa[(n & 0x0F)];
    str[2]        = hexa[(n & 0xF000) >> 12];
    str[3]        = hexa[(n & 0x0F00) >> 8];
    str[4]        = hexa[(n & 0xF00000) >> 20];
    str[5]        = hexa[(n & 0x0F0000) >> 16];
    str[6]        = '\0';
}

// Park

// Backlash
void Skywatcher::SetBacklashRA(unsigned long backlash)
{
    Backlash[Axis1] = backlash;
}

void Skywatcher::SetBacklashUseRA(bool usebacklash)
{
    UseBacklash[Axis1] = usebacklash;
}
void Skywatcher::SetBacklashDE(unsigned long backlash)
{
    Backlash[Axis2] = backlash;
}

void Skywatcher::SetBacklashUseDE(bool usebacklash)
{
    UseBacklash[Axis2] = usebacklash;
}
