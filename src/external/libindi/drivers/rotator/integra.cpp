/*
    Gemini Telescope Design Integra85 Focusing Rotator.
    Copyright (C) 2017 Hans Lambermont

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

#include "integra.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <termios.h>

#define INTEGRA_TIMEOUT_IN_S 5
#define INTEGRA_TEMPERATURE_LOOP_SKIPS 60
#define INTEGRA_TEMPERATURE_TRESHOLD_IN_C 0.1
#define INTEGRA_ROUNDING_FUDGE 0.001

#define POLLMS 1000
#define ROTATOR_TAB "Rotator"
#define SETTINGS_TAB "Settings"

std::unique_ptr<Integra> integra(new Integra());

void ISGetProperties(const char *dev)
{
    integra->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    integra->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    integra->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    integra->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                char *formats[], char *names[], int n)
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

void ISSnoopDevice (XMLEle *root)
{
    integra->ISSnoopDevice(root);
}

Integra::Integra() : RotatorInterface(this)
{
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
    SetRotatorCapability(ROTATOR_CAN_ABORT | ROTATOR_CAN_SYNC | ROTATOR_CAN_REVERSE);
    setFocuserConnection(CONNECTION_SERIAL);
}

bool Integra::initProperties()
{
    INDI::Focuser::initProperties();

    IUFillNumber(&MaxPositionN[0], "Steps", "Focuser", "%.f", 0, 188600., 0., 188600.);
    IUFillNumber(&MaxPositionN[1], "Steps", "Rotator", "%.f", 0, 61802.0, 0., 61802.);
    IUFillNumberVector(&MaxPositionNP, MaxPositionN, 2, getDeviceName(), "MAX_POSITION", "Max position",
                       SETTINGS_TAB, IP_RO, 0, IPS_IDLE);

    FocusSpeedN[0].min = 1;
    FocusSpeedN[0].max = 1;
    FocusSpeedN[0].value = 1;

    // Temperature Sensor
    IUFillNumber(&SensorN[SENSOR_TEMPERATURE], "TEMPERATURE", "Temperature (C)", "%.2f", -100, 100., 1., 0.);
    IUFillNumberVector(&SensorNP, SensorN, 1, getDeviceName(), "SENSORS", "Sensors", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE );

    // Home Find
    IUFillSwitch(&FindHomeS[HOMING_IDLE], "HOMING_IDLE", "Idle", ISS_ON);
    IUFillSwitch(&FindHomeS[HOMING_START], "HOMING_START", "Start", ISS_OFF);
    IUFillSwitch(&FindHomeS[HOMING_ABORT], "HOMING_ABORT", "Abort", ISS_OFF);
    IUFillSwitchVector(&FindHomeSP, FindHomeS, HOMING_COUNT, getDeviceName(), "HOMING", "Home at Center", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    // Relative and absolute movement
    FocusAbsPosN[0].min   = 0;
    FocusAbsPosN[0].max   = MaxPositionN[0].value;
    FocusAbsPosN[0].step  = MaxPositionN[0].value / 50.0;
    FocusAbsPosN[0].value = 0;

    FocusRelPosN[0].max   = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    FocusRelPosN[0].min   = 0;
    FocusRelPosN[0].step  = FocusRelPosN[0].max / 100.0;
    FocusRelPosN[0].value = 100;

    INDI::RotatorInterface::initProperties(ROTATOR_TAB);

    // Rotator Ticks
    IUFillNumber(&RotatorAbsPosN[0], "ROTATOR_ABSOLUTE_POSITION", "Ticks", "%.f", 0., 61802., 1., 0.);
    IUFillNumberVector(&RotatorAbsPosNP, RotatorAbsPosN, 1, getDeviceName(), "ABS_ROTATOR_POSITION", "Goto", ROTATOR_TAB, IP_RW, 0, IPS_IDLE );

    addDebugControl();

    updatePeriodMS = POLLMS;

    serialConnection->setDefaultPort("/dev/integra_focusing_rotator1");
    serialConnection->setDefaultBaudRate(Connection::Serial::B_115200);

    return true;
}

bool Integra::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&MaxPositionNP);
        // Focus
        defineNumber(&SensorNP);
        defineSwitch(&FindHomeSP);

        // Rotator
        INDI::RotatorInterface::updateProperties();
        defineNumber(&RotatorAbsPosNP);
    }
    else
    {
        deleteProperty(MaxPositionNP.name);
        // Focus
        deleteProperty(SensorNP.name);
        deleteProperty(FindHomeSP.name);

        // Rotator
        INDI::RotatorInterface::updateProperties();
        deleteProperty(RotatorAbsPosNP.name);
    }

    return true;
}

bool Integra::Handshake()
{
    if (Ack())
        return true;

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from Integra, please ensure Integra controller is powered and the port is correct.");
    return false;
}

const char * Integra::getDefaultName()
{
    return "Integra85";
}

void Integra::cleanPrint(const char *cmd, char *cleancmd)
{
    int len = strlen(cmd);
    int j = 0;
    for (int i = 0; i<=len; i++) {
        if (cmd[i] == 0xA) {
            cleancmd[j++] = '\\';
            cleancmd[j++] = 'n';
        } else if (cmd[i] == 0xD) {
            cleancmd[j++] = '\\';
            cleancmd[j++] = 'r';
        } else {
            cleancmd[j++] = cmd[i];
        }
    }
}

bool Integra::Ack()
{
    bool rcFirmware = getFirmware();
    bool rcType = getFocuserType();
    bool rcMaxPositionMotorFocus = getMaxPosition(MOTOR_FOCUS);
    if ( ! rcMaxPositionMotorFocus) // first communication attempt, try twice if needed
        rcMaxPositionMotorFocus = getMaxPosition(MOTOR_FOCUS);
    bool rcMaxPositionMotorRotator = getMaxPosition(MOTOR_ROTATOR);
    return (rcFirmware && rcType && rcMaxPositionMotorFocus && rcMaxPositionMotorRotator);
}

bool Integra::getFirmware()
{
    char resp[64] = "not available"; // TODO the device cannot report its firmware version yet
    DEBUGF(INDI::Logger::DBG_SESSION, "Firmware version %s", resp);
    return true;
}

bool Integra::getFocuserType()
{
    char resp[64] = "Integra85"; // TODO this is an assumption until the device can report its type
    DEBUGF(INDI::Logger::DBG_SESSION, "Focuser Type %s", resp);

    if (strcmp(resp, "Integra85") == 0)
    {
        RotatorAbsPosN[0].min = 0;
        RotatorAbsPosN[0].max = 61802;
    }
    rotatorTicksPerDegree = RotatorAbsPosN[0].max / 360.0;
    rotatorDegreesPerTick = 360.0 / RotatorAbsPosN[0].max;

    return true;
}

bool Integra::relativeGotoMotor(MotorType type, int32_t relativePosition)
{
    char cmd[16] = {0};
    char *MotorMoveCommand;
    char MotorMoveIn[] = "MI";
    char MotorMoveOut[] = "MO";

    DEBUGF(INDI::Logger::DBG_DEBUG, "Start relativeGotoMotor to %d ...", relativePosition);
    if (relativePosition > 0)
        MotorMoveCommand = MotorMoveOut;
    else
        MotorMoveCommand = MotorMoveIn;

    if (type == MOTOR_FOCUS) {
        if (relativePosition > 0) {
            if (lastFocuserPosition + relativePosition > MaxPositionN[MOTOR_FOCUS].value) {
                int newRelativePosition = MaxPositionN[MOTOR_FOCUS].value - lastFocuserPosition;
                DEBUGF(INDI::Logger::DBG_SESSION, "Focus position change %d clipped to %d to stay at MAX %d",
                       relativePosition, newRelativePosition, MaxPositionN[MOTOR_FOCUS].value);
                relativePosition = newRelativePosition;
            }
        } else {
            if ((int32_t )lastFocuserPosition + relativePosition < 0) {
                int newRelativePosition = -lastFocuserPosition;
                DEBUGF(INDI::Logger::DBG_SESSION, "Focus position change %d clipped to %d to stay at MIN 0",
                       relativePosition, newRelativePosition);
                relativePosition = newRelativePosition;
            }
        }
    } else if (type == MOTOR_ROTATOR) {
        if (relativePosition > 0) {
            if (lastRotatorPosition + relativePosition > MaxPositionN[MOTOR_ROTATOR].value) {
                int newRelativePosition = MaxPositionN[MOTOR_ROTATOR].value - lastRotatorPosition;
                DEBUGF(INDI::Logger::DBG_SESSION, "Rotator position change %d clipped to %d to stay at MAX %d",
                       relativePosition, newRelativePosition, MaxPositionN[MOTOR_ROTATOR].value);
                relativePosition = newRelativePosition;
            }
        } else {
            if (lastRotatorPosition + relativePosition < - MaxPositionN[MOTOR_ROTATOR].value) {
                int newRelativePosition = - MaxPositionN[MOTOR_ROTATOR].value - lastRotatorPosition;
                DEBUGF(INDI::Logger::DBG_SESSION, "Rotator position change %d clipped to %d to stay at MIN %d",
                       relativePosition, newRelativePosition, - MaxPositionN[MOTOR_ROTATOR].value);
                relativePosition = newRelativePosition;
            }
        }
    }

    snprintf(cmd, 16, "@%s%d,%d\r\n", MotorMoveCommand, type+1, abs(relativePosition));
    return genericIntegraCommand(__FUNCTION__, cmd, "M", nullptr);
}

bool Integra::gotoMotor(MotorType type, int32_t position)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Start gotoMotor to %d", position);
    if (type == MOTOR_FOCUS)
    {
        return relativeGotoMotor(type, position - lastFocuserPosition);
    }
    else if (type == MOTOR_ROTATOR)
    {
        return relativeGotoMotor(type, position - lastRotatorPosition);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: MotorType %d is unknown.", __FUNCTION__, type);
    }
    return false;
}

bool Integra::getPosition(MotorType type)
{
    char cmd[16] = {0};
    snprintf(cmd, 16, "@PR%d,0\r\n", type+1);
    char res[16] = {0};
    if ( ! genericIntegraCommand(__FUNCTION__, cmd, "P", res))
    {
        return false;
    }
    int position = -1e6;
    position = atoi(res);
    if (position != -1e6)
    {
        if (type == MOTOR_FOCUS) {
            if (FocusAbsPosN[0].value != position) {
                int position_from = (int) FocusAbsPosN[0].value;
                int position_to = position;
                if (haveReadFocusPositionAtLeastOnce) {
                    DEBUGF(INDI::Logger::DBG_DEBUG, "Focus position changed from %d to %d", position_from, position_to);
                } else {
                    DEBUGF(INDI::Logger::DBG_DEBUG, "Focus position is %d", position_to);
                }
                FocusAbsPosN[0].value = position;
            }
        }
        else if (type == MOTOR_ROTATOR) {
            if (RotatorAbsPosN[0].value != position) {
                int position_from = (int) RotatorAbsPosN[0].value;
                int position_to = position;
                if (haveReadRotatorPositionAtLeastOnce) {
                    DEBUGF(INDI::Logger::DBG_DEBUG, "Rotator changed angle from %.2f to %.2f, position from %d to %d",
                           rotatorTicksToDegrees(position_from), rotatorTicksToDegrees(position_to), position_from, position_to);
                } else {
                    DEBUGF(INDI::Logger::DBG_DEBUG, "Rotator angle is %.2f, position is %d",
                           rotatorTicksToDegrees(position_to), position_to);
                }
                RotatorAbsPosN[0].value = position;
            }
        }
        else {
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: motor type %d is unknown", __FUNCTION__, type);
        }

        return true;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Invalid Position! %d", position);
    return false;
}

bool Integra::ISNewSwitch (const char * dev, const char * name, ISState * states, char * names[], int n)
{
    if(strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, FindHomeSP.name) == 0)
        {
            IUUpdateSwitch(&FindHomeSP, states, names, n);
            int index = IUFindOnSwitchIndex(&FindHomeSP);
            switch (index)
            {
                case HOMING_IDLE:
                    DEBUG(INDI::Logger::DBG_SESSION, "Homing state is IDLE");
                    FindHomeS[HOMING_IDLE].s = ISS_ON;
                    FindHomeSP.s = IPS_OK;
                    break;
                case HOMING_START:
                    if (findHome()) {
                        FindHomeSP.s = IPS_BUSY;
                        FindHomeS[HOMING_START].s = ISS_ON;
                        DEBUG(INDI::Logger::DBG_WARNING,
                              "Homing process can take up to 2 minutes. You cannot control the unit until the process is fully complete.");
                    } else {
                        FindHomeSP.s = IPS_ALERT;
                        FindHomeS[HOMING_START].s = ISS_OFF;
                        DEBUG(INDI::Logger::DBG_ERROR, "Failed to start homing process.");
                    }
                    break;
                case HOMING_ABORT:
                    if (abortHome()) {
                        FindHomeSP.s = IPS_IDLE;
                        FindHomeS[HOMING_ABORT].s = ISS_ON;
                        DEBUG(INDI::Logger::DBG_WARNING, "Homing aborted");
                    } else {
                        FindHomeSP.s = IPS_ALERT;
                        FindHomeS[HOMING_ABORT].s = ISS_OFF;
                        DEBUG(INDI::Logger::DBG_ERROR, "Failed to abort homing process.");
                    }
                    break;
                default:
                    FindHomeSP.s = IPS_ALERT;
                    IDSetSwitch(&FindHomeSP, "Unknown homing index %d", index);
                    return false;
            }
            IDSetSwitch(&FindHomeSP, nullptr);
            return true;
        }
        else if (strstr(name, "ROTATOR"))
        {
            if (INDI::RotatorInterface::processSwitch(dev, name, states, names, n))
                return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool Integra::ISNewNumber (const char * dev, const char * name, double values[], char * names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, RotatorAbsPosNP.name) == 0)
        {
            RotatorAbsPosNP.s = (gotoMotor(MOTOR_ROTATOR, static_cast<int32_t>(values[0])) ? IPS_BUSY : IPS_ALERT);
            IDSetNumber(&RotatorAbsPosNP, nullptr);
            if (RotatorAbsPosNP.s == IPS_BUSY)
                DEBUGF(INDI::Logger::DBG_DEBUG, "Rotator moving from %d to %.f ticks...", lastRotatorPosition, values[0]);
            return true;
        }
        else if (strstr(name, "ROTATOR"))
        {
            if (INDI::RotatorInterface::processNumber(dev, name, values, names, n))
                return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

IPState Integra::MoveAbsFocuser(uint32_t targetTicks)
{
    targetPosition = targetTicks;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Focuser will move absolute from %d to %d ...", lastFocuserPosition, targetTicks);

    bool rc = false;
    rc = gotoMotor(MOTOR_FOCUS, targetPosition);
    if (!rc)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState Integra::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double newPosition = 0;
    bool rc = false;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Focuser will move in direction %d relative %d ticks...", dir, ticks);

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    rc = gotoMotor(MOTOR_FOCUS, newPosition);
    if (!rc)
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

void Integra::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(POLLMS);
        return;
    }

    bool rc = false;
    bool savePositionsToEEPROM = false;

    // #1 If we're homing, we check if homing is complete as we cannot check for anything else
    if (FindHomeSP.s == IPS_BUSY)
    {
        if (isHomingComplete())
        {
            FindHomeS[0].s = ISS_OFF;
            FindHomeSP.s = IPS_OK;
            IDSetSwitch(&FindHomeSP, nullptr);

            DEBUG(INDI::Logger::DBG_SESSION, "Homing is complete");
            // Next read positions and save to EEPROM :
            haveReadFocusPositionAtLeastOnce = false;
            haveReadRotatorPositionAtLeastOnce = false;
        } else {
            DEBUG(INDI::Logger::DBG_DEBUG, "Homing");
        }

        SetTimer(POLLMS);
        return;
    }

    // #2 Get Temperature, only read this when no motors are active, and about once per minute
    if (FocusAbsPosNP.s != IPS_BUSY && FocusRelPosNP.s != IPS_BUSY
        && RotatorAbsPosNP.s != IPS_BUSY
        && timeToReadTemperature <= 0) {
        rc = getTemperature();
        if ( ! rc)
            rc = getTemperature();
        if (rc) {
            timeToReadTemperature = INTEGRA_TEMPERATURE_LOOP_SKIPS;
            if (fabs(SensorN[SENSOR_TEMPERATURE].value - lastTemperature) > INTEGRA_TEMPERATURE_TRESHOLD_IN_C) {
                lastTemperature = SensorN[SENSOR_TEMPERATURE].value;
                IDSetNumber(&SensorNP, nullptr);
            }
        }
    } else {
        timeToReadTemperature--;
    }

    // #5 Focus Position & Status
    if (haveReadFocusPositionAtLeastOnce == false || FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if ( ! isMotorMoving(MOTOR_FOCUS))
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Focuser stopped");
            FocusAbsPosNP.s = IPS_OK;
            FocusRelPosNP.s = IPS_OK;
            rc = getPosition(MOTOR_FOCUS);
            if (rc) {
                if (FocusAbsPosN[0].value != lastFocuserPosition) {
                    lastFocuserPosition = FocusAbsPosN[0].value;
                    IDSetNumber(&FocusAbsPosNP, nullptr);
                    IDSetNumber(&FocusRelPosNP, nullptr);
                    if (haveReadFocusPositionAtLeastOnce) {
                        DEBUGF(INDI::Logger::DBG_SESSION, "Focuser reached requested position %d", lastFocuserPosition);
                    } else {
                        DEBUGF(INDI::Logger::DBG_SESSION, "Focuser position is %d", lastFocuserPosition);
                        haveReadFocusPositionAtLeastOnce = true;
                    }
                    savePositionsToEEPROM = true;
                }
            }
        } else {
            DEBUG(INDI::Logger::DBG_DEBUG, "Focusing");
        }
    }

    // #6 Rotator Position & Status
    if (haveReadRotatorPositionAtLeastOnce == false || RotatorAbsPosNP.s == IPS_BUSY)
    {
        if ( ! isMotorMoving(MOTOR_ROTATOR))
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Rotator stopped");
            RotatorAbsPosNP.s = IPS_OK;
            GotoRotatorNP.s = IPS_OK;
            rc = getPosition(MOTOR_ROTATOR);
            if (rc) {
                if (RotatorAbsPosN[0].value != lastRotatorPosition) {
                    lastRotatorPosition = RotatorAbsPosN[0].value;
                    GotoRotatorN[0].value = rotatorTicksToDegrees(lastRotatorPosition); //range360(RotatorAbsPosN[0].value / rotatorTicksPerDegree);
                    IDSetNumber(&RotatorAbsPosNP, nullptr);
                    IDSetNumber(&GotoRotatorNP, nullptr);
                    if (haveReadRotatorPositionAtLeastOnce)
                        DEBUGF(INDI::Logger::DBG_SESSION, "Rotator reached requested angle %.2f, position %d",
                               rotatorTicksToDegrees(lastRotatorPosition), lastRotatorPosition);
                    else {
                        DEBUGF(INDI::Logger::DBG_SESSION, "Rotator is at angle %.2f, position %d",
                               rotatorTicksToDegrees(lastRotatorPosition), lastRotatorPosition);
                        haveReadRotatorPositionAtLeastOnce = true;
                    }
                    savePositionsToEEPROM = true;
                }
            }
        } else {
            DEBUG(INDI::Logger::DBG_DEBUG, "Rotating");
        }
    }

    if (savePositionsToEEPROM) {
        saveToEEPROM();
    }
    SetTimer(POLLMS);
}

bool Integra::AbortFocuser()
{
    return stopMotor(MOTOR_FOCUS);
}

bool Integra::stopMotor(MotorType type)
{
    // TODO (if focuser?) handle CR 2
    char cmd[16] = {0};
    snprintf(cmd, 16, "@SW%d,0\r\n", type+1);
    if (genericIntegraCommand(__FUNCTION__, cmd, "S", nullptr))
    {
        if (type == MOTOR_FOCUS) {
            haveReadFocusPositionAtLeastOnce = false;
        } else {
            haveReadRotatorPositionAtLeastOnce = false;
        }
        return true;
    }
    return false;
}

bool Integra::isMotorMoving(MotorType type) {
    char res[16] = {0};
    if ( ! genericIntegraCommand(__FUNCTION__, "X", nullptr, res)) {
        return false;
    }
    if (type == MOTOR_FOCUS) {
        if (res[0] == '1') {
            DEBUG(INDI::Logger::DBG_DEBUG, "Focus motor is running");
            return true;
        } else {
            DEBUG(INDI::Logger::DBG_DEBUG, "Focus motor is not running");
            return false;
        }
    } else {
        // bug, both motors return 1 at res[0] when running
        //  return (res[0] == '2');
        if (res[0] == '1') {
            DEBUG(INDI::Logger::DBG_DEBUG, "Rotator motor is running");
            return true;
        } else {
            DEBUG(INDI::Logger::DBG_DEBUG, "Rotator motor is not running");
            return false;
        }
    }
}

bool Integra::getMaxPosition(MotorType type)
{
    char cmd[16] = {0};
    char res[16] = {0};
    snprintf(cmd, 16, "@RR%d,0\r\n", type+1);
    if ( ! genericIntegraCommand(__FUNCTION__, cmd, "R", res))
    {
        return false;
    }
    int position = atoi(res);
    MaxPositionN[type].value = position;
    DEBUGF(INDI::Logger::DBG_SESSION, "Motor %d max position is %d", type, position);
    return true;
}

bool Integra::saveToEEPROM()
{
    return genericIntegraCommand(__FUNCTION__, "@ZW\r\n", "Z", nullptr);
}

bool Integra::getTemperature() {
    char res[16] = {0};
    if (genericIntegraCommand(__FUNCTION__, "@TR\r\n", "T", res))
    {
        SensorN[SENSOR_TEMPERATURE].value = strtod(res, NULL);
        return true;
    }
    return false;
}

bool Integra::findHome()
{
    return genericIntegraCommand(__FUNCTION__, "@CS1,0\r\n", "CS", nullptr);
}

bool Integra::abortHome()
{
    return genericIntegraCommand(__FUNCTION__, "@CE1,0\r\n", "CE", nullptr);
}

bool Integra::isHomingComplete()
{
    char res[16] = {0};
    if (genericIntegraCommand(__FUNCTION__, "@CR1,0\r\n", "C", res))
    {
        return (res[0] == '1');
    }
    return false;
}

bool Integra::saveConfigItems(FILE *fp)
{
    Focuser::saveConfigItems(fp);
    return true;
}

// Integra position 0..61802 ticks , angle 0..360 deg, position 0 corresponds to 180 deg
// We need to map the Integra frame to that of the IndiRotatorInterface.
// INDI rotatorInterface: Only absolute position Rotators are supported.
// Angle is ranged from 0 to 360 increasing clockwise when looking at the back of the camera.
IPState Integra::MoveRotator(double angle)
{
    uint32_t p1 = lastRotatorPosition;
    uint32_t p2 = rotatorDegreesToTicks(angle);

    DEBUGF(INDI::Logger::DBG_SESSION, "MoveRotator from %.2f to %.2f degrees, from position %d to %d ...",
           rotatorTicksToDegrees(lastRotatorPosition), angle, p1, p2);
    bool rc = relativeGotoMotor(MOTOR_ROTATOR, p2 - p1);
    if (rc)
    {
        RotatorAbsPosNP.s = IPS_BUSY;
        IDSetNumber(&RotatorAbsPosNP, nullptr);
        return IPS_BUSY;
    }

    return IPS_ALERT;
}

bool Integra::AbortRotator()
{
    bool rc = stopMotor(MOTOR_ROTATOR);
    if (rc && RotatorAbsPosNP.s != IPS_OK)
    {
        RotatorAbsPosNP.s = IPS_OK;
        IDSetNumber(&RotatorAbsPosNP, nullptr);
    }

    return rc;
}

uint32_t Integra::rotatorDegreesToTicks(double angle)
{
    uint32_t position = 61802 / 2;
    if (angle >= 0.0 && angle <= 180.0) {
        position = (uint32_t) lround(61802.0 - (180.0 - angle) / rotatorDegreesPerTick);
    } else if (angle > 180 && angle <= 360) {
        position = (uint32_t) lround(61802.0 - (540.0 - angle) / rotatorDegreesPerTick);
    } else {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %.2f is out of range", __FUNCTION__, angle);
    }
    return position;
}

double Integra::rotatorTicksToDegrees(uint32_t ticks)
{
    double degrees = range360(180.0 + ticks * rotatorDegreesPerTick + INTEGRA_ROUNDING_FUDGE);
    return degrees;
}

bool Integra::SyncRotator(double angle) {
    char cmd[16] = {0};
    uint32_t position = rotatorDegreesToTicks(angle);
    snprintf(cmd, 16, "@PW2,%d\r\n", position);
    if (genericIntegraCommand(__FUNCTION__, cmd, "P", nullptr))
    {
        haveReadRotatorPositionAtLeastOnce = false;
        return true;
    }
    return false;
}

bool Integra::ReverseRotator(bool)
{
    return genericIntegraCommand(__FUNCTION__, "@IW2,0\r\n", "I", nullptr);
}

bool Integra::genericIntegraCommand(const char *name, const char *cmd, const char *expectStart, char *returnValueString)
{
    char cmdnocrlf[16] = {0};
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char res[16] = {0};
    char errstr[MAXRBUF];

    cleanPrint(cmd, cmdnocrlf);
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD %s <%s>", name, cmdnocrlf);

    tcflush(PortFD, TCIOFLUSH);
    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", name, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', INTEGRA_TIMEOUT_IN_S, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", name, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES %s <%s>", name, res);

    // check begin of result string
    if (expectStart != nullptr)
    {
        int expectStrlen = strlen(expectStart);
        for (int i=0; i<expectStrlen; i++)
        {
            if (res[i] != expectStart[i])
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "%s error: invalid response <%s>", name, res);
                return false;
            }
        }
    }
    // check end of result string
    if (res[nbytes_read-1] != '#')
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: invalid response <%s>", name, res);
        return false;
    }
    res[nbytes_read-1] = '\0';  // wipe the #

    if (returnValueString != nullptr) {
        // handle the exception case X that returns a result that does NOT start with an identifier
        if (res[0] < 'A' || res[0] > 'Z') {
            strcpy(returnValueString, res);
        } else {
            strcpy(returnValueString, res+1);
        }
    }

    return true;
}