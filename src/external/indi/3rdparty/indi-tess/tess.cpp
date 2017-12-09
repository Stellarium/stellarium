/*
    NACHO MAS 2013 (mas.ignacio at gmail.com)
    Driver for TESS custom hardware. See README

    Base on tutorial_Four
    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>

#include <libnova/transform.h>
#include <libnova/julian_day.h>
#include <libnova/utility.h>

/* Our driver header */
#include "tess.h"
#include "tess_algebra.h"
#include "config.h"

using namespace std;

/* Our inditess auto pointer */
unique_ptr<inditess> inditess_prt(new inditess());

const int POLLMS = 100; // Period of update, 0.1 second.

/**************************************************************************************
** Send client definitions of all properties.
***************************************************************************************/
/**************************************************************************************
**
***************************************************************************************/
void ISGetProperties(const char *dev)
{
    inditess_prt->ISGetProperties(dev);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    inditess_prt->ISNewSwitch(dev, name, states, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    inditess_prt->ISNewText(dev, name, texts, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    inditess_prt->ISNewNumber(dev, name, values, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    inditess_prt->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

/**************************************************************************************
** LX200 Basic constructor
***************************************************************************************/
inditess::inditess()
{
    IDLog("inditess driver start...\n");
    setVersion(TESS_VERSION_MAJOR, TESS_VERSION_MINOR);
}

/**************************************************************************************
**
***************************************************************************************/
inditess::~inditess()
{
}

void inditess::TimerHit()
{
    char readBuffer[TESS_MSG_SIZE];
    static char buffer[TESS_BUFFER];
    char TESSmsg[TESS_MSG_SIZE];
    int nbytes_read = 0;
    int startInx    = 0;
    char dummy[24];
    static int pointer      = -1;
    static int fail_counter = 0;
    static double az = 0, alt = 0;
    double ra, dec;
    if (isConnected() == false)
        return;
    if (fd <= 0)
        return;

    tty_read(fd, readBuffer, TESS_MSG_SIZE, TESS_TIMEOUT, &nbytes_read);
    //IDLog("nbytes_read:%u\n",nbytes_read);

    if (nbytes_read == 0)
    {
        fail_counter++;
        //IDLog("TESS not responding. Fail number:%u of %u max allowed\n",fail_counter,MAX_CONNETIONS_FAILS);
    }
    else
    {
        fail_counter = 0;
    }
    if (fail_counter == MAX_CONNETIONS_FAILS)
    {
        fail_counter               = 0;
        ISwitchVectorProperty *svp = getSwitch("CONNECTION");
        svp->sp[0].s               = ISS_OFF;
        svp->sp[1].s               = ISS_ON;
        svp->s                     = IPS_ALERT;
        tty_disconnect(fd);
        IDSetSwitch(svp, "TESS Conection loose.DISCONNECTING\n");
        IDLog("TESS Conection loose\n");
        return;
    }
    startInx = -1;
    //IDLog("readBuffer:%s\n",readBuffer);

    //Not enougth data in the previous loop. Concatenate
    if (pointer != -1 && pointer < TESS_MSG_SIZE)
    {
        //IDLog("1Buffer:%s\n",buffer);
        //IDLog("Pointer:%u\n",pointer);
        strncpy(&buffer[pointer], readBuffer, TESS_MSG_SIZE - pointer);
        //IDLog("2Buffer:%s\n",buffer);
        pointer = pointer + nbytes_read;
    }

    //Searching for star
    if (pointer == -1)
    {
        for (int i = 1; i < TESS_MSG_SIZE; i++)
        {
            if (readBuffer[i] == 'f')
            {
                startInx = i - 1;
                pointer  = nbytes_read - startInx;
                //IDLog("Pointer:%u\n",pointer);
                buffer[0] = '\0';
                strncpy(buffer, &readBuffer[startInx], pointer);
                //IDLog("0Buffer:%s\n",buffer);
                //IDLog("startInx:%i\n",startInx);
                break;
            }
        }
    }

    //Check if enought data if not wait next loop
    if (pointer >= TESS_MSG_SIZE)
    {
        pointer = -1;
    }
    else
    {
        return;
    }
    strcpy(TESSmsg, buffer);
    //IDLog("TESSmsg:%s\n",TESSmsg);

    //Decode msg
    strncpy(dummy, TESSmsg + 3, 6);
    fH = atof(dummy);
    if (TESSmsg[2] == 'm')
    {
        fH = atof(dummy) / 1000;
    }
    strncpy(dummy, TESSmsg + 13, 6);
    tO = atof(dummy) / 100;
    strncpy(dummy, TESSmsg + 23, 6);
    tA = atof(dummy) / 100;
    strncpy(dummy, TESSmsg + 33, 6);
    aX = atof(dummy);
    strncpy(dummy, TESSmsg + 43, 6);
    aY = atof(dummy);
    strncpy(dummy, TESSmsg + 53, 6);
    aZ = atof(dummy);
    strncpy(dummy, TESSmsg + 63, 6);
    mX = atof(dummy);
    strncpy(dummy, TESSmsg + 73, 6);
    mY = atof(dummy);
    strncpy(dummy, TESSmsg + 83, 6);
    mZ = atof(dummy);
    // IDLog("fH:%f tO:%f tA:%f aX:%f aY:%f aZ:%f mX:%f mY:%f aZ:%f\n",fH,tO,tA,aX,aY,aZ,mX,mY,mZ);

    //Do calculation
    //Set sensor facing orientation in board
    p.x = 0;
    p.y = 1;
    p.z = 0;

    //Load sensor values
    // placa orientada hacia arriba con el conector cerca en el lado proximo
    // X hacia el cable
    // Y hacia la derecha
    // Z hacia arriba
    a_avg.x = aY;
    a_avg.y = -aX;
    a_avg.z = aZ;
    m_avg.x = mY;
    m_avg.y = -mX;
    m_avg.z = mZ;

    //Heading
    get_heading(&a_avg, &m_avg, &p, &heading);
    //LOWPASS Filter
    //y[i] := y[i-1] + α * (x[i] - y[i-1])
    if ((fabs(az - heading.z) > ALTAZ_FAST_CHANGE) || (fabs(alt - heading.y) > ALTAZ_FAST_CHANGE))
    {
        alt = heading.y;
        az  = heading.z;
    }
    alt = alt + filter * (heading.y - alt);
    az  = az + filter * (heading.z - az);

    //RA-DEC calculation
    HrztoEqu(alt, az, &ra, &dec);
    //IDLog("RA/DEC:%f %f\n",ra,dec);
    //Update properties
    std::vector<INDI::Property *> *pAll = getProperties();
    for (int i = 0; i < pAll->size(); i++)
    {
        const char *name;
        const char *label;
        INDI_PROPERTY_TYPE type;
        name  = pAll->at(i)->getName();
        label = pAll->at(i)->getLabel();
        type  = pAll->at(i)->getType();
        //IDLog("PROP:%s\n",name);
        //DIGITAL INPUT
        if (type == INDI_LIGHT)
        {
            ILightVectorProperty *lvp = getLight(name);
            for (int i = 0; i < lvp->nlp; i++)
            {
                ILight *lqp = &lvp->lp[i];
                if (!lqp)
                    return;
            }
            IDSetLight(lvp, NULL);
        }

        //ANALOG
        if (type == INDI_NUMBER)
        {
            INumberVectorProperty *nvp = getNumber(name);

            for (int i = 0; i < nvp->nnp; i++)
            {
                INumber *eqp = &nvp->np[i];
                if (!eqp)
                    return;

                if (!strcmp(eqp->name, "fH"))
                {
                    eqp->value = fH;
                }
                else if (!strcmp(eqp->name, "tO"))
                {
                    eqp->value = tO;
                }
                else if (!strcmp(eqp->name, "tA"))
                {
                    eqp->value = tA;
                }
                else if (!strcmp(eqp->name, "aX"))
                {
                    eqp->value = aX;
                }
                else if (!strcmp(eqp->name, "aY"))
                {
                    eqp->value = aY;
                }
                else if (!strcmp(eqp->name, "aZ"))
                {
                    eqp->value = aZ;
                }
                else if (!strcmp(eqp->name, "mX"))
                {
                    eqp->value = mX;
                }
                else if (!strcmp(eqp->name, "mY"))
                {
                    eqp->value = mY;
                }
                else if (!strcmp(eqp->name, "mZ"))
                {
                    eqp->value = mZ;
                }
                else if (!strcmp(eqp->name, "SKYBRIGHT"))
                {
                    eqp->value = HzToMag(fH);
                }
                else if (!strcmp(eqp->name, "AMBTEMP"))
                {
                    eqp->value = tA;
                }
                else if (!strcmp(eqp->name, "SKYTEMP"))
                {
                    eqp->value = tO;
                }
                else if (!strcmp(eqp->name, "ALT"))
                {
                    eqp->value = alt;
                }
                else if (!strcmp(eqp->name, "AZ"))
                {
                    eqp->value = az;
                }
                else if (!strcmp(eqp->name, "RA"))
                {
                    eqp->value = ra / 15;
                }
                else if (!strcmp(eqp->name, "DEC"))
                {
                    eqp->value = dec;
                }
            }
            IDSetNumber(nvp, NULL);
        }

        //TEXT
        if (type == INDI_TEXT)
        {
            ITextVectorProperty *tvp = getText(name);

            for (int i = 0; i < tvp->ntp; i++)
            {
                IText *eqp = &tvp->tp[i];
                if (!eqp)
                    return;

                if (eqp->aux0 == NULL)
                    continue;
                strcpy(eqp->text, (char *)eqp->aux0);
                //IDLog("%s.%s TEXT: %s \n",tvp->name,eqp->name,eqp->text);
                IDSetText(tvp, NULL);
            }
        }
    }

    SetTimer(POLLMS);
}

/**************************************************************************************
** Initialize all properties & set default values.
**************************************************************************************/
bool inditess::initProperties()
{
    // This is the default driver skeleton file location
    // Convention is: drivername_sk_xml
    // Default location is /usr/share/indi

    struct stat st;
    char skelFileName[255];
    strcpy(skelFileName, DEFAULT_SKELETON_FILE);

    setVersion(TESS_VERSION_MAJOR, TESS_VERSION_MINOR);

    char *skel = getenv("INDISKEL");
    if (skel)
    {
        IDLog("Building from %s skeleton\n", skel);
        strcpy(skelFileName, skel);
        buildSkeleton(skel);
    }
    else if (stat(skelFileName, &st) == 0)
    {
        IDLog("Building from %s skeleton\n", skelFileName);
        buildSkeleton(skelFileName);
    }
    else
    {
        IDLog(
            "No skeleton file was specified. Set environment variable INDISKEL to the skeleton path and try again.\n");
    }
    // Optional: Add aux controls for configuration, debug & simulation that get added in the Options tab
    //           of the driver.
    //addAuxControls();

    DefaultDevice::initProperties();

    return true;
}

/**************************************************************************************
** Define Basic properties to clients.
***************************************************************************************/
void inditess::ISGetProperties(const char *dev)
{
    static int configLoaded = 0;

    // Ask the default driver first to send properties.
    INDI::DefaultDevice::ISGetProperties(dev);

    configLoaded = 1;
}

/**************************************************************************************
** Process Text properties
***************************************************************************************/
bool inditess::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDeviceName()))
        return false;

    ITextVectorProperty *tProp = getText(name);
    // Device Port Text
    if (!strcmp(tProp->name, "DEVICE_PORT"))
    {
        if (IUUpdateText(tProp, texts, names, n) < 0)
            return false;

        tProp->s = IPS_OK;
        tProp->s = IPS_IDLE;
        IDSetText(tProp, "Port updated.");

        return true;
    }

    return false;
}

/**************************************************************************************
**
***************************************************************************************/
bool inditess::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDeviceName()))
        return false;

    INumberVectorProperty *nvp = getNumber(name);
    //IDLog("%s\n",nvp->name);

    if (!nvp)
        return false;

    if (isConnected() == false)
    {
        nvp->s = IPS_ALERT;
        IDSetNumber(nvp, "Cannot change property while device is disconnected.");
        return false;
    }

    bool change = false;
    for (int i = 0; i < n; i++)
    {
        INumber *eqp = IUFindNumber(nvp, names[i]);
        //IDLog("%s\n",eqp->name);

        if (!eqp)
            return false;
        IUUpdateNumber(nvp, values, names, n);
        if (!strcmp(eqp->name, "Lat"))
        {
            Lat = eqp->value;
            IDLog("New LAT:%f\n", Lat);
        }
        else if (!strcmp(eqp->name, "Lon"))
        {
            Lon = eqp->value;
            IDLog("New Lon:%f\n", Lon);
        }
        else if (!strcmp(eqp->name, "filter"))
        {
            filter = eqp->value;
            IDLog("New filter value:%f\n", filter);
        }
        else if (!strcmp(eqp->name, "ALT"))
        {
            alt_offset = eqp->value;
            IDLog("Sync ALT offset:%f\n", alt_offset);
        }
        else if (!strcmp(eqp->name, "AZ"))
        {
            az_offset = eqp->value;
            IDLog("Sync AZ offset:%f\n", az_offset);
        }

        change = true;
    }

    if (change)
    {
        IDSetNumber(nvp, "Property:%s update\n", nvp->name);
        return true;
    }
    else
    {
        return false;
    }
}

/**************************************************************************************
**
***************************************************************************************/
bool inditess::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int lightState = 0;
    int lightIndex = 0;

    // ignore if not ours //
    if (strcmp(dev, getDeviceName()))
        return false;

    if (INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n) == true)
        return true;

    ISwitchVectorProperty *svp = getSwitch(name);
    IDLog("NAME:%s\n", name);
    IDLog("%s\n", svp->name);

    if (isConnected() == false)
    {
        svp->s = IPS_ALERT;
        IDSetSwitch(svp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (!svp)
        return false;

    for (int i = 0; i < svp->nsp; i++)
    {
        ISwitch *sqp = &svp->sp[i];
    }

    IUUpdateSwitch(svp, states, names, n);
    return true;
}

bool inditess::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                         char *formats[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()))
        return false;

    const char *testBLOB = "This is a test BLOB from the driver";

    IBLOBVectorProperty *bvp = getBLOB(name);

    if (!bvp)
        return false;

    if (isConnected() == false)
    {
        bvp->s = IPS_ALERT;
        IDSetBLOB(bvp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (!strcmp(bvp->name, "BLOB Test"))
    {
        IUUpdateBLOB(bvp, sizes, blobsizes, blobs, formats, names, n);

        IBLOB *bp = IUFindBLOB(bvp, names[0]);

        if (!bp)
            return false;

        IDLog("Recieved BLOB with name %s, format %s, and size %d, and bloblen %d\n", bp->name, bp->format, bp->size,
              bp->bloblen);

        char *blobBuffer = new char[bp->bloblen + 1];
        strncpy(blobBuffer, ((char *)bp->blob), bp->bloblen);
        blobBuffer[bp->bloblen] = '\0';

        IDLog("BLOB Content:\n##################################\n%s\n##################################\n",
              blobBuffer);

        delete[] blobBuffer;
    }

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool inditess::Connect()
{
    ITextVectorProperty *tProp = getText("DEVICE_PORT");
    ISwitchVectorProperty *svp = getSwitch("CONNECTION");
    if (tty_connect(tProp->tp[0].text, 9600, 8, 0, 1, &fd) == TTY_OK)
    {
        //IDLog("svp->s:%u\n",svp->s);
        IDSetSwitch(svp, "TESS BOARD CONNECTED.\n");
        IDLog("TESS BOARD CONNECTED.\n");
        //Init Input properties
        INumberVectorProperty *nvp = getNumber("LATLON");
        Lat                        = nvp->np[0].value;
        Lon                        = nvp->np[1].value;
        nvp                        = getNumber("Filter");
        filter                     = nvp->np[0].value;

        SetTimer(POLLMS);
    }
    else
    {
        IDLog("TESS BOARD FAIL TO CONNECT. CHECK PORT NAME\n");
        IDSetSwitch(svp, "TESS BOARD FAIL TO CONNECT. CHECK PORT NAME\n");
        return false;
    }
}

bool inditess::Disconnect()
{
    ISwitchVectorProperty *svp = getSwitch("CONNECTION");
    tty_disconnect(fd);
    IDLog("TESS BOARD DISCONNECTED.\n");
    IDSetSwitch(svp, "DISCONNECTED\n");
    return true;
}

const char *inditess::getDefaultName()
{
    return "TESS";
}

float inditess::HzToMag(float HzTSL)
{
    float mv;
    mv = HzTSL / 230.0; // Iradiancia en (uW/cm2)/10
    if (mv > 0)
    {
        mv = mv * 0.000001;                 //irradiancia en W/cm2
        mv = -1 * (log10(mv) / log10(2.5)); //log en base 2.5
        if (mv < 0)
            mv = 24;
    }
    else
        mv = 24;
    return mv;
}

int inditess::HrztoEqu(double alt, double az, double *ra, double *dec)
{
    struct ln_lnlat_posn observer;
    struct ln_hrz_posn hrz;
    struct ln_equ_posn equ;

    double JD;
    struct ln_date date;

    /* 
         * observers position
         * longitude is measured positively eastwards
         */
    observer.lng = Lon;
    observer.lat = Lat;
    //IDLog("Lat/Lon %f %f\n",observer.lat,observer.lng);
    /*
	  * Horizontal coordinates. Libnova az:
          0 deg = South, 90 deg = West, 180 deg = Nord, 270 deg = East
          conversion needed
	 */
    az = az - 180.;
    if (az <= 0)
        az = az + 360.;
    hrz.az  = az;
    hrz.alt = alt;

    JD = ln_get_julian_from_sys();
    ln_get_equ_from_hrz(&hrz, &observer, JD, &equ);

    *ra  = equ.ra;
    *dec = equ.dec;
    return 0;
}
