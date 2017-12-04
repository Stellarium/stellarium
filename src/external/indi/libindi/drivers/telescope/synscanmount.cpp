/*******************************************************************************
  Copyright(c) 2010 Gerry Rozema. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "synscanmount.h"

#include "indicom.h"

#include <libnova/transform.h>
// libnova specifies round() on old systems and it collides with the new gcc 5.x/6.x headers
#define HAVE_ROUND
#include <libnova/utility.h>

#include <cmath>
#include <memory>
#include <cstring>

#define SYNSCAN_SLEW_RATES 9

// We declare an auto pointer to Synscan.
std::unique_ptr<SynscanMount> synscan(new SynscanMount());

void ISGetProperties(const char *dev)
{
    synscan->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    synscan->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    synscan->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    synscan->ISNewNumber(dev, name, values, names, n);
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
    synscan->ISSnoopDevice(root);
}

SynscanMount::SynscanMount()
{
    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_ABORT | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO |
                               TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION,
                           SYNSCAN_SLEW_RATES);
    SetParkDataType(PARK_RA_DEC_ENCODER);
    strncpy(LastParkRead, "", 1);
}

bool SynscanMount::Connect()
{
    if (isConnected())
        return true;

    bool Ret = INDI::Telescope::Connect();

    if (Ret)
    {
        AnalyzeHandset();
    }
    return Ret;
}

const char *SynscanMount::getDefaultName()
{
    return "SynScan";
}

bool SynscanMount::initProperties()
{
    bool Ret = INDI::Telescope::initProperties();

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_ABORT | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO |
                           TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION,
                           SYNSCAN_SLEW_RATES);
    SetParkDataType(PARK_RA_DEC_ENCODER);

    //  probably want to debug this
    addDebugControl();
    addConfigurationControl();

    // Set up property variables
    IUFillText(&BasicMountInfo[(int)MountInfoItems::FwVersion], "FW_VERSION", "Firmware version", "-");
    IUFillText(&BasicMountInfo[(int)MountInfoItems::MountCode], "MOUNT_CODE", "Mount code", "-");
    IUFillText(&BasicMountInfo[(int)MountInfoItems::AlignmentStatus], "ALIGNMENT_STATUS", "Alignment status", "-");
    IUFillText(&BasicMountInfo[(int)MountInfoItems::GotoStatus], "GOTO_STATUS", "Goto status", "-");
    IUFillText(&BasicMountInfo[(int)MountInfoItems::MountPointingStatus], "MOUNT_POINTING_STATUS",
               "Mount pointing status", "-");
    IUFillText(&BasicMountInfo[(int)MountInfoItems::TrackingMode], "TRACKING_MODE", "Tracking mode", "-");
    IUFillTextVector(&BasicMountInfoV, BasicMountInfo, 6, getDeviceName(), "BASIC_MOUNT_INFO",
                     "Mount information", MountInfoPage.c_str(), IP_RO, 60, IPS_IDLE);

    return Ret;
}

void SynscanMount::ISGetProperties(const char *dev)
{
    /* First we let our parent populate */
    INDI::Telescope::ISGetProperties(dev);

    if (isConnected())
    {
        UpdateMountInformation(false);
        defineText(&BasicMountInfoV);
    }
}

bool SynscanMount::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool SynscanMount::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool SynscanMount::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                             char *formats[], char *names[], int n)
{
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool SynscanMount::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

bool SynscanMount::updateProperties()
{
    INDI::Telescope::updateProperties();
    DEBUG(INDI::Logger::DBG_SESSION, "Update Properties");

    if (isConnected())
    {
        UpdateMountInformation(false);
        defineText(&BasicMountInfoV);
    }
    else
    {
        deleteProperty(BasicMountInfoV.name);
    }

    return true;
}

int SynscanMount::HexStrToInteger(const std::string &str)
{
    return std::stoi(str, nullptr, 16);
}

bool SynscanMount::AnalyzeHandset()
{
    bool rc = true;
    int caps = 0;
    int tmp = 0;
    int bytesWritten = 0;
    int bytesRead, numread;
    char str[20];

    caps = GetTelescopeCapability();

    rc = ReadLocation();
    if (rc)
    {
        CanSetLocation = true;
        ReadTime();
    }

    bytesRead = 0;
    memset(str, 0, 20);
    tty_write(PortFD, "J", 1, &bytesWritten);
    tty_read(PortFD, str, 2, 2, &bytesRead);

    // Read the handset version
    bytesRead = 0;
    memset(str, 0, 20);
    tty_write(PortFD, "V", 1, &bytesWritten);
    tty_read(PortFD, str, 7, 2, &bytesRead);
    if (bytesRead == 3)
    {
        int tmp1 { 0 }, tmp2 { 0 };

        tmp  = str[0];
        tmp1 = str[1];
        tmp2 = str[2];
        FirmwareVersion = tmp2;
        FirmwareVersion /= 100;
        FirmwareVersion += tmp1;
        FirmwareVersion /= 100;
        FirmwareVersion += tmp;
    } else {
        FirmwareVersion = (double)HexStrToInteger(std::string(&str[0], 2));
        FirmwareVersion += (double)HexStrToInteger(std::string(&str[2], 2)) / 100;
        FirmwareVersion += (double)HexStrToInteger(std::string(&str[4], 2)) / 10000;
    }
    DEBUGF(INDI::Logger::DBG_SESSION, "Firmware version: %lf", FirmwareVersion);
    if (FirmwareVersion < 3.38 || (FirmwareVersion >= 4.0 && FirmwareVersion < 4.38))
    {
        IDMessage(nullptr, "Update Synscan firmware to V3.38/V4.38 or above");
        DEBUG(INDI::Logger::DBG_SESSION, "Too old firmware version!");
    } else {
        NewFirmware = true;
    }
    HandsetFwVersion = std::to_string(FirmwareVersion);

    memset(str, 0, 2);
    tty_write(PortFD, "m", 1, &bytesWritten);
    tty_read(PortFD, str, 2, 2, &bytesRead);
    if (bytesRead == 2)
    {
        // This workaround is needed because the firmware 3.39 sends these bytes swapped.
        if (str[1] == '#')
            MountCode = (int)*reinterpret_cast<unsigned char*>(&str[0]);
        else
            MountCode = (int)*reinterpret_cast<unsigned char*>(&str[1]);
    }

    // Check the tracking status
    memset(str, 0, 2);
    tty_write(PortFD, "t", 1, &bytesWritten);
    numread = tty_read(PortFD, str, 2, 2, &bytesRead);
    if (str[1] == '#' && (int)str[0] != 0)
    {
        TrackState = SCOPE_TRACKING;
    }

    SetTelescopeCapability(caps, SYNSCAN_SLEW_RATES);

    if (InitPark())
    {
        SetAxis1ParkDefault(0);
        SetAxis2ParkDefault(90);
    }
    else
    {
        SetAxis1Park(0);
        SetAxis2Park(90);
        SetAxis1ParkDefault(0);
        SetAxis2ParkDefault(90);
    }
    return true;
}

bool SynscanMount::ReadScopeStatus()
{
    char str[20];
    int bytesWritten, bytesRead;
    int numread;
    double ra, dec;
    long unsigned int n1, n2;

    tty_write(PortFD, "Ka", 2, &bytesWritten); //  test for an echo
    tty_read(PortFD, str, 2, 2, &bytesRead);   //  Read 2 bytes of response
    if (str[1] != '#')
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Synscan Mount not responding");
        // Usually, Abort() recovers the communication
        RecoverTrials++;
        Abort();
//        HasFailed = true;
        return false;
    }
    RecoverTrials = 0;

    /*
    //  With 3.37 firmware, on the older line of eq6 mounts
    //  The handset does not always initialize the communication with the motors correctly
    //  We can check for this condition by querying the motors for firmware version
    //  and if it returns zero, it means we need to power cycle the handset
    //  and try again after it restarts again

        if(HasFailed) {
            int v1,v2;
    	//fprintf(stderr,"Calling passthru command to get motor firmware versions\n");
            v1=PassthruCommand(0xfe,0x11,1,0,2);
            v2=PassthruCommand(0xfe,0x10,1,0,2);
            fprintf(stderr,"Motor firmware versions %d %d\n",v1,v2);
            if((v1==0)||(v2==0)) {
                IDMessage(getDeviceName(),"Cannot proceed");
                IDMessage(getDeviceName(),"Handset is responding, but Motors are Not Responding");
                return false;
    	}
            //  if we get here, both motors are responding again
            //  so the problem is solved
    	HasFailed=false;
        }
    */

    //  on subsequent passes, we just need to read the time
    if (HasTime())
    {
        ReadTime();
    }
    if (HasLocation())
    {
        //  this flag is set when we get a new lat/long from the host
        //  so we should go thru the read routine once now, so things update
        //  correctly in the client displays
        if (ReadLatLong)
        {
            ReadLocation();
        }
    }

    // Query mount information
    memset(str, 0, 2);
    tty_write(PortFD, "J", 1, &bytesWritten);
    numread = tty_read(PortFD, str, 2, 2, &bytesRead);
    if (str[1] == '#')
    {
        AlignmentStatus = std::to_string((int)str[0]);
    }
    memset(str, 0, 2);
    tty_write(PortFD, "L", 1, &bytesWritten);
    numread = tty_read(PortFD, str, 2, 2, &bytesRead);
    if (str[1] == '#')
    {
        GotoStatus = str[0];
    }
    memset(str, 0, 2);
    tty_write(PortFD, "p", 1, &bytesWritten);
    numread = tty_read(PortFD, str, 2, 2, &bytesRead);
    if (str[1] == '#')
    {
        MountPointingStatus = str[0];
    }
    memset(str, 0, 2);
    tty_write(PortFD, "t", 1, &bytesWritten);
    numread = tty_read(PortFD, str, 2, 2, &bytesRead);
    if (str[1] == '#')
    {
        if ((int)str[0] == 0)
            TrackingMode = "Tracking off";
        if ((int)str[0] == 1)
            TrackingMode = "Alt/Az tracking";
        if ((int)str[0] == 2)
            TrackingMode = "EQ tracking";
        if ((int)str[0] == 3)
            TrackingMode = "PEC mode";
    }

    UpdateMountInformation(true);

    if (TrackState == SCOPE_SLEWING)
    {
        //  We have a slew in progress
        //  lets see if it's complete
        //  This only works for ra/dec goto commands
        //  The goto complete flag doesn't trip for ALT/AZ commands
        if (GotoStatus != "0")
        {
            //  Nothing to do here
        }
        else
        {
            if (TrackState == SCOPE_PARKING)
                TrackState = SCOPE_PARKED;
        }
    }
    if (TrackState == SCOPE_PARKING)
    {
        if (FirmwareVersion == 4.103500)
        {
            //  With this firmware the correct way
            //  is to check the slewing flat
            memset(str, 0, 3);
            tty_write(PortFD, "L", 1, &bytesWritten);
            numread = tty_read(PortFD, str, 2, 3, &bytesRead);
            if (str[0] != 48)
            {
                //  Nothing to do here
            }
            else
            {
                if (NumPark++ < 2)
                {
                    Park();
                }
                else
                {
                    TrackState = SCOPE_PARKED;
                    SetParked(true);
                }
            }
        }
        else
        {
            //  ok, lets try read where we are
            //  and see if we have reached the park position
            //  newer firmware versions dont read it back the same way
            //  so we watch now to see if we get the same read twice in a row
            //  to confirm that it has stopped moving
            memset(str, 0, 20);
            tty_write(PortFD, "z", 1, &bytesWritten);
            numread = tty_read(PortFD, str, 18, 2, &bytesRead);

            //IDMessage(getDeviceName(),"Park Read %s %d",str,StopCount);

            if (strncmp((char *)str, LastParkRead, 18) == 0)
            {
                //  We find that often after it stops from park
                //  it's off the park position by a small amount
                //  issuing another park command gets a small movement and then
                if (++StopCount > 2)
                {
                    if (NumPark++ < 2)
                    {
                        StopCount = 0;
                        //IDMessage(getDeviceName(),"Sending park again");
                        Park();
                    }
                    else
                    {
                        TrackState = SCOPE_PARKED;
                        //ParkSP.s=IPS_OK;
                        //IDSetSwitch(&ParkSP,nullptr);
                        //IDMessage(getDeviceName(),"Telescope is Parked.");
                        SetParked(true);
                    }
                }
                else
                {
                    //StopCount=0;
                }
            }
            else
            {
                StopCount = 0;
            }
            strncpy(LastParkRead, str, sizeof(str));
        }
    }

    memset(str, 0, 20);
    tty_write(PortFD, "e", 1, &bytesWritten);
    numread = tty_read(PortFD, str, 18, 1, &bytesRead);
    if (bytesRead != 18)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Read current position failed");
        return false;
    }

    sscanf(str, "%lx,%lx#", &n1, &n2);
    ra  = (double)n1 / 0x100000000 * 24.0;
    dec = (double)n2 / 0x100000000 * 360.0;
    CurrentRA  = ra;
    CurrentDEC = dec;

    //  Now feed the rest of the system with corrected data
    NewRaDec(ra, dec);

    if (TrackState == SCOPE_SLEWING && MountCode >= 128 && (SlewTargetAz != -1 || SlewTargetAlt != -1))
    {
        ln_hrz_posn CurrentAltAz { 0, 0 };
        double DiffAlt { 0 };
        double DiffAz { 0 };

        CurrentAltAz = GetAltAzPosition(ra, dec);
        DiffAlt = CurrentAltAz.alt-SlewTargetAlt;
        if (SlewTargetAlt != -1 && std::abs(DiffAlt) > 0.01)
        {
            int NewRate = 2;

            if (std::abs(DiffAlt) > 4)
            {
                NewRate = 9;
            } else
            if (std::abs(DiffAlt) > 1.2)
            {
                NewRate = 7;
            } else
            if (std::abs(DiffAlt) > 0.5)
            {
                NewRate = 5;
            } else
            if (std::abs(DiffAlt) > 0.2)
            {
                NewRate = 4;
            } else
            if (std::abs(DiffAlt) > 0.025)
            {
                NewRate = 3;
            }
            DEBUGF(INDI::Logger::DBG_DEBUG, "Slewing Alt axis: %1.3f-%1.3f -> %1.3f (speed: %d)",
                   CurrentAltAz.alt, SlewTargetAlt, CurrentAltAz.alt-SlewTargetAlt, CustomNSSlewRate);
            if (NewRate != CustomNSSlewRate)
            {
                if (DiffAlt < 0)
                {
                    CustomNSSlewRate = NewRate;
                    MoveNS(DIRECTION_NORTH, MOTION_START);
                } else {
                    CustomNSSlewRate = NewRate;
                    MoveNS(DIRECTION_SOUTH, MOTION_START);
                }
            }
        } else
        if (SlewTargetAlt != -1 && std::abs(DiffAlt) < 0.01)
        {
            MoveNS(DIRECTION_NORTH, MOTION_STOP);
            SlewTargetAlt = -1;
            DEBUG(INDI::Logger::DBG_SESSION, "Slewing on Alt axis finished");
        }
        DiffAz = CurrentAltAz.az-SlewTargetAz;
        if (DiffAz < -180)
            DiffAz = (DiffAz+360)*2;
        else
        if (DiffAz > 180)
            DiffAz = (DiffAz-360)*2;
        if (SlewTargetAz != -1 && std::abs(DiffAz) > 0.01)
        {
            int NewRate = 2;

            if (std::abs(DiffAz) > 4)
            {
                NewRate = 9;
            } else
            if (std::abs(DiffAz) > 1.2)
            {
                NewRate = 7;
            } else
            if (std::abs(DiffAz) > 0.5)
            {
                NewRate = 5;
            } else
            if (std::abs(DiffAz) > 0.2)
            {
                NewRate = 4;
            } else
            if (std::abs(DiffAz) > 0.025)
            {
                NewRate = 3;
            }
            DEBUGF(INDI::Logger::DBG_DEBUG, "Slewing Az axis: %1.3f-%1.3f -> %1.3f (speed: %d)",
                   CurrentAltAz.az, SlewTargetAz, CurrentAltAz.az-SlewTargetAz, CustomWESlewRate);
            if (NewRate != CustomWESlewRate)
            {
                if (DiffAz > 0)
                {
                    CustomWESlewRate = NewRate;
                    MoveWE(DIRECTION_WEST, MOTION_START);
                } else {
                    CustomWESlewRate = NewRate;
                    MoveWE(DIRECTION_EAST, MOTION_START);
                }
            }
        } else
        if (SlewTargetAz != -1 && std::abs(DiffAz) < 0.01)
        {
            MoveWE(DIRECTION_WEST, MOTION_STOP);
            SlewTargetAz = -1;
            DEBUG(INDI::Logger::DBG_SESSION, "Slewing on Az axis finished");
        }
        if (SlewTargetAz == -1 && SlewTargetAlt == -1)
        {
            StartTrackMode();
        }
    }
    return true;
}

bool SynscanMount::StartTrackMode()
{
    char str[20];
    int numread, bytesWritten, bytesRead;

    TrackState = SCOPE_TRACKING;
    DEBUG(INDI::Logger::DBG_SESSION, "Tracking started");
    // Start tracking
    str[0] = 'T';
    // Check the mount type to choose tracking mode
    if (MountCode >= 128)
    {
        // Alt/Az tracking mode
        str[1] = 1;
    } else {
        // EQ tracking mode
        str[1] = 2;
    }
    tty_write(PortFD, str, 2, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 2, &bytesRead);
    if (bytesRead != 1 || str[0] != '#')
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Timeout waiting for scope to start tracking.");
        return false;
    }
    return true;
}

bool SynscanMount::Goto(double ra, double dec)
{
    char str[20];
    int bytesWritten, bytesRead;
    ln_hrz_posn TargetAltAz { 0, 0 };

    tty_write(PortFD, "Ka", 2, &bytesWritten); //  test for an echo
    tty_read(PortFD, str, 2, 2, &bytesRead);   //  Read 2 bytes of response
    if (str[1] != '#')
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Wrong answer from the mount");
        //  this is not a correct echo
        //  so we are not talking to a mount properly
        return false;
    }
    TrackState = SCOPE_SLEWING;
    // EQ mount has a different Goto mode
    if (MountCode < 128)
    {
        int n1 = ra * 0x1000000 / 24;
        int n2 = dec * 0x1000000 / 360;
        int numread;

        n1 = n1 << 8;
        n2 = n2 << 8;
        sprintf((char*)str, "r%08X,%08X", n1, n2);
        tty_write(PortFD, str, 18, &bytesWritten);
        memset(&str[18], 0, 1);
        DEBUGF(INDI::Logger::DBG_DEBUG, "Goto - ra: %g de: %g", ra, dec);
        numread = tty_read(PortFD, str, 1, 60, &bytesRead);
        if (bytesRead != 1 || str[0] != '#')
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Timeout waiting for scope to complete goto.");
            return false;
        }
        return true;
    }
    TargetAltAz = GetAltAzPosition(ra, dec);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Goto - ra: %g de: %g (az: %g alt: %g)", ra, dec,
           TargetAltAz.az, TargetAltAz.alt);
    SlewTargetAz = TargetAltAz.az;
    SlewTargetAlt = TargetAltAz.alt;
    return true;
}

bool SynscanMount::Park()
{
    char str[20];
    int numread, bytesWritten, bytesRead;

    strncpy(LastParkRead, "", 1);
    memset(str, 0, 3);
    tty_write(PortFD, "Ka", 2, &bytesWritten); //  test for an echo
    tty_read(PortFD, str, 2, 2, &bytesRead);   //  Read 2 bytes of response
    if (str[1] != '#')
    {
        //  this is not a correct echo
        //  so we are not talking to a mount properly
        return false;
    }
    //  Now we stop tracking
    str[0] = 'T';
    str[1] = 0;
    tty_write(PortFD, str, 2, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 60, &bytesRead);
    if (bytesRead != 1 || str[0] != '#')
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Timeout waiting for scope to stop tracking.");
        return false;
    }

    //sprintf((char *)str,"b%08X,%08X",0x0,0x40000000);
    tty_write(PortFD, "b00000000,40000000", 18, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 60, &bytesRead);
    if (bytesRead != 1 || str[0] != '#')
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Timeout waiting for scope to respond to park.");
        return false;
    }

    TrackState = SCOPE_PARKING;
    if (NumPark == 0)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Parking Mount...");
    }
    StopCount = 0;
    return true;
}

bool SynscanMount::UnPark()
{
    SetParked(false);
    NumPark = 0;
    return true;
}

bool SynscanMount::SetCurrentPark()
{
    DEBUG(INDI::Logger::DBG_SESSION, "Setting arbitrary park positions is not supported yet.");
    return false;
}

bool SynscanMount::SetDefaultPark()
{
    // By default az to north, and alt to pole
    DEBUG(INDI::Logger::DBG_DEBUG, "Setting Park Data to Default.");
    SetAxis1Park(0);
    SetAxis2Park(90);

    return true;
}

bool SynscanMount::Abort()
{
    if (TrackState == SCOPE_IDLE || RecoverTrials >= 3)
        return true;

    char str[20];
    int numread, bytesWritten, bytesRead;

    DEBUG(INDI::Logger::DBG_SESSION, "Abort any motions");
    TrackState = SCOPE_IDLE;
    SlewTargetAlt = -1;
    SlewTargetAz = -1;
    CustomNSSlewRate = -1;
    CustomWESlewRate = -1;
    // Stop tracking
    str[0] = 'T';
    str[1] = 0;
    tty_write(PortFD, str, 2, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 2, &bytesRead);
    if (bytesRead != 1 || str[0] != '#')
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Timeout waiting for scope to stop tracking.");
        return false;
    }

    // Hmmm twice only stops it
    tty_write(PortFD, "M", 1, &bytesWritten);
    tty_read(PortFD, str, 1, 1, &bytesRead); //  Read 1 bytes of response

    tty_write(PortFD, "M", 1, &bytesWritten);
    tty_read(PortFD, str, 1, 1, &bytesRead); //  Read 1 bytes of response

    return true;
}

bool SynscanMount::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    if (command != MOTION_START)
    {
        PassthruCommand(37, 17, 2, 0, 0);
    }
    else
    {
        int tt = (CustomNSSlewRate == -1 ? SlewRate : CustomNSSlewRate);

        tt = tt << 16;
        if (dir != DIRECTION_NORTH)
        {
            PassthruCommand(37, 17, 2, tt, 0);
        }
        else
        {
            PassthruCommand(36, 17, 2, tt, 0);
        }
    }

    return true;
}

bool SynscanMount::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    if (command != MOTION_START)
    {
        PassthruCommand(37, 16, 2, 0, 0);
    }
    else
    {
        int tt = (CustomWESlewRate == -1 ? SlewRate : CustomWESlewRate);

        tt = tt << 16;
        if (dir != DIRECTION_WEST)
        {
            PassthruCommand(36, 16, 2, tt, 0);
        }
        else
        {
            PassthruCommand(37, 16, 2, tt, 0);
        }
    }

    return true;
}

bool SynscanMount::SetSlewRate(int s)
{
    SlewRate = s + 1;
    return true;
}

int SynscanMount::PassthruCommand(int cmd, int target, int msgsize, int data, int numReturn)
{
    char test[20];
    int bytesRead, bytesWritten;
    char a, b, c;
    int tt = data;

    a  = tt % 256;
    tt = tt >> 8;
    b  = tt % 256;
    tt = tt >> 8;
    c  = tt % 256;

    //  format up a passthru command
    memset(test, 0, 20);
    test[0] = 80;      // passhtru
    test[1] = msgsize; // set message size
    test[2] = target;  // set the target
    test[3] = cmd;     // set the command
    test[4] = c;       // set data bytes
    test[5] = b;
    test[6] = a;
    test[7] = numReturn;

    tty_write(PortFD, test, 8, &bytesWritten);
    memset(test, 0, 20);
    tty_read(PortFD, test, numReturn + 1, 2, &bytesRead);
    if (numReturn > 0)
    {
        int retval = 0;
        retval     = test[0];
        if (numReturn > 1)
        {
            retval = retval << 8;
            retval += test[1];
        }
        if (numReturn > 2)
        {
            retval = retval << 8;
            retval += test[2];
        }
        return retval;
    }

    return 0;
}

bool SynscanMount::ReadTime()
{
    char str[20];
    int bytesWritten = 0, bytesRead = 0;

    //  lets see if this hand controller responds to a time request
    bytesRead = 0;
    tty_write(PortFD, "h", 1, &bytesWritten);
    tty_read(PortFD, str, 9, 2, &bytesRead);
    if (str[8] == '#')
    {
        ln_zonedate localTime;
        ln_date utcTime;
        int offset, daylightflag;

        localTime.hours   = str[0];
        localTime.minutes = str[1];
        localTime.seconds = str[2];
        localTime.months  = str[3];
        localTime.days    = str[4];
        localTime.years   = str[5];
        localTime.gmtoff  = str[6];
        offset            = str[6];
        daylightflag =
            str[7]; //  this is the daylight savings flag in the hand controller, needed if we did not set the time
        localTime.years += 2000;
        localTime.gmtoff *= 3600;
        //  now convert to utc
        ln_zonedate_to_date(&localTime, &utcTime);

        //  now we have time from the hand controller, we need to set some variables
        int sec;
        char utc[100];
        char ofs[10];
        sec = (int)utcTime.seconds;
        sprintf(utc, "%04d-%02d-%dT%d:%02d:%02d", utcTime.years, utcTime.months, utcTime.days, utcTime.hours,
                utcTime.minutes, sec);
        if (daylightflag == 1)
            offset = offset + 1;
        sprintf(ofs, "%d", offset);

        IUSaveText(&TimeT[0], utc);
        IUSaveText(&TimeT[1], ofs);
        TimeTP.s = IPS_OK;
        IDSetText(&TimeTP, nullptr);

        return true;
    }
    return false;
}

bool SynscanMount::ReadLocation()
{
    char str[20];
    int bytesWritten = 0, bytesRead = 0;

    tty_write(PortFD, "Ka", 2, &bytesWritten); //  test for an echo
    tty_read(PortFD, str, 2, 2, &bytesRead);   //  Read 2 bytes of response
    if (str[1] != '#')
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Bad echo in ReadLocation");
    }
    else
    {
        //  lets see if this hand controller responds to a location request
        bytesRead = 0;
        tty_write(PortFD, "w", 1, &bytesWritten);
        tty_read(PortFD, str, 9, 2, &bytesRead);
        if (str[8] == '#')
        {
            double lat, lon;
            //  lets parse this data now
            int a, b, c, d, e, f, g, h;
            a = str[0];
            b = str[1];
            c = str[2];
            d = str[3];
            e = str[4];
            f = str[5];
            g = str[6];
            h = str[7];
            //fprintf(stderr,"Pos %d:%d:%d  %d:%d:%d\n",a,b,c,e,f,g);

            double t1, t2, t3;

            t1  = c;
            t2  = b;
            t3  = a;
            t1  = t1 / 3600.0;
            t2  = t2 / 60.0;
            lat = t1 + t2 + t3;

            t1  = g;
            t2  = f;
            t3  = e;
            t1  = t1 / 3600.0;
            t2  = t2 / 60.0;
            lon = t1 + t2 + t3;

            if (d == 1)
                lat = lat * -1;
            if (h == 1)
                lon = 360 - lon;
            LocationN[LOCATION_LATITUDE].value  = lat;
            LocationN[LOCATION_LONGITUDE].value = lon;
            IDSetNumber(&LocationNP, nullptr);
            //  We dont need to keep reading this one on every cycle
            //  only need to read it when it's been changed
            ReadLatLong = false;
            return true;
        }
        else
        {
            DEBUG(INDI::Logger::DBG_SESSION, "Mount does not support setting location");
        }
    }
    return false;
}

bool SynscanMount::updateTime(ln_date *utc, double utc_offset)
{
    char str[20];
    int bytesWritten = 0, bytesRead = 0;

    //  start by formatting a time for the hand controller
    //  we are going to set controller to local time
    //
    struct ln_zonedate ltm;

    ln_date_to_zonedate(utc, &ltm, utc_offset * 3600.0);

    int yr = ltm.years;

    yr = yr % 100;

    str[0] = 'H';
    str[1] = ltm.hours;
    str[2] = ltm.minutes;
    str[3] = ltm.seconds;
    str[4] = ltm.months;
    str[5] = ltm.days;
    str[6] = yr;
    str[7] = utc_offset; //  offset from utc so hand controller is running in local time
    str[8] = 0;          //  and no daylight savings adjustments, it's already included in the offset
    //  lets write a time to the hand controller
    bytesRead = 0;
    tty_write(PortFD, str, 9, &bytesWritten);
    tty_read(PortFD, str, 1, 2, &bytesRead);
    if (str[0] != '#')
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Invalid return from set time");
    }
    return true;
}

bool SynscanMount::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);

    char str[20];
    int bytesWritten = 0, bytesRead = 0;
    int s = 0;
    bool IsWest = false;
    double tmp = 0;

    ln_lnlat_posn p1 { 0, 0 };
    lnh_lnlat_posn p2;

    LocationN[LOCATION_LATITUDE].value  = latitude;
    LocationN[LOCATION_LONGITUDE].value = longitude;
    IDSetNumber(&LocationNP, nullptr);

    if (!CanSetLocation)
    {
        return true;
    }
    else
    {
        if (longitude > 180)
        {
            p1.lng = 360.0 - longitude;
            IsWest = true;
        }
        else
        {
            p1.lng = longitude;
        }
        p1.lat = latitude;
        ln_lnlat_to_hlnlat(&p1, &p2);
        DEBUGF(INDI::Logger::DBG_SESSION, "Update location - latitude %d:%d:%1.2f longitude %d:%d:%1.2f\n",
               p2.lat.degrees, p2.lat.minutes, p2.lat.seconds, p2.lng.degrees, p2.lng.minutes, p2.lng.seconds);

        str[0] = 'W';
        str[1] = p2.lat.degrees;
        str[2] = p2.lat.minutes;
        tmp    = p2.lat.seconds + 0.5;
        s      = (int)tmp; //  put in an int that's rounded
        str[3] = s;
        if (p2.lat.neg == 0)
        {
            str[4] = 0;
        } else {
            str[4] = 1;
        }

        str[5] = p2.lng.degrees;
        str[6] = p2.lng.minutes;
        s      = (int)(p2.lng.seconds + 0.5); //  make an int, that's rounded
        str[7] = s;
        if (IsWest)
            str[8] = 1;
        else
            str[8] = 0;
        //  All formatted, now send to the hand controller;
        bytesRead = 0;
        tty_write(PortFD, str, 9, &bytesWritten);
        tty_read(PortFD, str, 1, 2, &bytesRead);
        if (str[0] != '#')
        {
            DEBUG(INDI::Logger::DBG_SESSION, "Invalid response for location setting");
        }
        //  want to read it on the next cycle, so we update the fields in the client
        ReadLatLong = true;

        return true;
    }
}

bool SynscanMount::Sync(double ra, double dec)
{
    bool IsTrackingBeforeSync = (TrackState == SCOPE_TRACKING);

    // Abort any motion before syncing
    Abort();

    DEBUGF(INDI::Logger::DBG_SESSION, "Sync %g %g -> %g %g", CurrentRA, CurrentDEC, ra, dec);

    char str[20];
    int numread, bytesWritten, bytesRead;
    ln_hrz_posn TargetAltAz { 0, 0 };

    TargetAltAz = GetAltAzPosition(ra, dec);
    if (isDebug())
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Sync - ra: %g de: %g to az: %g alt: %g", ra, dec,
               TargetAltAz.az, TargetAltAz.alt);
    } else {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Sync - ra: %g de: %g to az: %g alt: %g", ra, dec,
               TargetAltAz.az, TargetAltAz.alt);
    }
    // Assemble the Reset Position command for Az axis
    int Az = (int)(TargetAltAz.az*16777216 / 360);

    str[0] = 'P';
    str[1] = 4;
    str[2] = 16;
    str[3] = 4;
    *reinterpret_cast<unsigned char*>(&str[4]) = (unsigned char)(Az / 65536);
    Az -= (Az / 65536)*65536;
    *reinterpret_cast<unsigned char*>(&str[5]) = (unsigned char)(Az / 256);
    Az -= (Az / 256)*256;
    *reinterpret_cast<unsigned char*>(&str[6]) = (unsigned char)Az;
    str[7] = 0;
    tty_write(PortFD, str, 8, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 3, &bytesRead);
    // Assemble the Reset Position command for Alt axis
    int Alt = (int)(TargetAltAz.alt*16777216 / 360);

    str[0] = 'P';
    str[1] = 4;
    str[2] = 17;
    str[3] = 4;
    *reinterpret_cast<unsigned char*>(&str[4]) = (unsigned char)(Alt / 65536);
    Alt -= (Alt / 65536)*65536;
    *reinterpret_cast<unsigned char*>(&str[5]) = (unsigned char)(Alt / 256);
    Alt -= (Alt / 256)*256;
    *reinterpret_cast<unsigned char*>(&str[6]) = (unsigned char)Alt;
    str[7] = 0;
    tty_write(PortFD, str, 8, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 2, &bytesRead);

    // Pass the sync command to the handset
    int n1 = ra * 0x1000000 / 24;
    int n2 = dec * 0x1000000 / 360;

    n1 = n1 << 8;
    n2 = n2 << 8;
    sprintf((char*)str, "s%08X,%08X", n1, n2);
    memset(&str[18], 0, 1);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Send Sync command to the handset (%s)", str);
    tty_write(PortFD, str, 18, &bytesWritten);
    numread = tty_read(PortFD, str, 1, 60, &bytesRead);
    if (bytesRead != 1 || str[0] != '#')
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Timeout waiting for scope to complete syncing.");
        return false;
    }
    // Start tracking again
    if (IsTrackingBeforeSync)
        StartTrackMode();

    return true;
}

ln_hrz_posn SynscanMount::GetAltAzPosition(double ra, double dec)
{
    ln_lnlat_posn Location { 0, 0 };
    ln_equ_posn Eq { 0, 0 };
    ln_hrz_posn AltAz { 0, 0 };

    // Set the current location
    Location.lat = LocationN[LOCATION_LATITUDE].value;
    Location.lng = LocationN[LOCATION_LONGITUDE].value;

    Eq.ra  = ra*360.0 / 24.0;
    Eq.dec = dec;
    ln_get_hrz_from_equ(&Eq, &Location, ln_get_julian_from_sys(), &AltAz);
    AltAz.az -= 180;
    if (AltAz.az < 0)
        AltAz.az += 360;

    return AltAz;
}

void SynscanMount::UpdateMountInformation(bool inform_client)
{
    bool BasicMountInfoHasChanged = false;
    std::string MountCodeStr = std::to_string(MountCode);

    if (std::string(BasicMountInfo[(int)MountInfoItems::FwVersion].text) != HandsetFwVersion)
    {
        IUSaveText(&BasicMountInfo[(int)MountInfoItems::FwVersion], HandsetFwVersion.c_str());
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[(int)MountInfoItems::MountCode].text) != MountCodeStr)
    {
        IUSaveText(&BasicMountInfo[(int)MountInfoItems::MountCode], MountCodeStr.c_str());
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[(int)MountInfoItems::AlignmentStatus].text) != AlignmentStatus)
    {
        IUSaveText(&BasicMountInfo[(int)MountInfoItems::AlignmentStatus], AlignmentStatus.c_str());
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[(int)MountInfoItems::GotoStatus].text) != GotoStatus)
    {
        IUSaveText(&BasicMountInfo[(int)MountInfoItems::GotoStatus], GotoStatus.c_str());
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[(int)MountInfoItems::MountPointingStatus].text) != MountPointingStatus)
    {
        IUSaveText(&BasicMountInfo[(int)MountInfoItems::MountPointingStatus], MountPointingStatus.c_str());
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[(int)MountInfoItems::TrackingMode].text) != TrackingMode)
    {
        IUSaveText(&BasicMountInfo[(int)MountInfoItems::TrackingMode], TrackingMode.c_str());
        BasicMountInfoHasChanged = true;
    }
    if (BasicMountInfoHasChanged && inform_client)
        IDSetText(&BasicMountInfoV, nullptr);
}
