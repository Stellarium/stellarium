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

#include "scope-limits.h"

#include "../eqmod.h"

#include <indicom.h>

#include <cstring>

#include <algorithm> // std::sort
#include <wordexp.h>

bool cmphorizonpoint(horizonpoint h1, horizonpoint h2)
{
    return (h1.az < h2.az);
}

HorizonLimits::HorizonLimits(INDI::Telescope *t)
{
    telescope    = t;
    horizon      = new std::vector<horizonpoint>;
    horizonindex = -1;
    strcpy(errorline, "Bad number format line     ");
    sline = errorline + 23;
    HorizonInitialized=false;
}

HorizonLimits::~HorizonLimits()
{
    delete (horizon);
    horizon      = NULL;
    horizonindex = -1;
}

const char *HorizonLimits::getDeviceName()
{
    return telescope->getDeviceName();
}
void HorizonLimits::Reset()
{
    if (horizon)
        horizon->erase(horizon->begin(), horizon->end());
}
void HorizonLimits::Init()
{
    if (!HorizonInitialized)
    {
        char *res = LoadDataFile(IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text);
	if (res)
	{
	    DEBUGF(INDI::Logger::DBG_WARNING, "Can not load HorizonLimits Data File %s: %s",
		     IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text, res);
	}
	else
	{
	    DEBUGF(INDI::Logger::DBG_SESSION, "HorizonLimits: Data loaded from file %s",
		     IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text);
	}
    }
    HorizonInitialized = true;
}

bool HorizonLimits::initProperties()
{
    /* Load properties from the skeleton file */

    telescope->buildSkeleton("indi_eqmod_scope_limits_sk.xml");

    HorizonLimitsDataFileTP      = telescope->getText("HORIZONLIMITSDATAFILE");
    HorizonLimitsDataFitsBP      = telescope->getBLOB("HORIZONLIMITSDATAFITS");
    HorizonLimitsPointNP         = telescope->getNumber("HORIZONLIMITSPOINT");
    HorizonLimitsTraverseSP      = telescope->getSwitch("HORIZONLIMITSTRAVERSE");
    HorizonLimitsManageSP        = telescope->getSwitch("HORIZONLIMITSMANAGE");
    HorizonLimitsFileOperationSP = telescope->getSwitch("HORIZONLIMITSFILEOPERATION");
    HorizonLimitsOnLimitSP       = telescope->getSwitch("HORIZONLIMITSONLIMIT");
    HorizonLimitsLimitGotoSP     = telescope->getSwitch("HORIZONLIMITSLIMITGOTO");

    return true;
}

void HorizonLimits::ISGetProperties()
{
    if (telescope->isConnected())
    {
        telescope->defineText(HorizonLimitsDataFileTP);
        telescope->defineBLOB(HorizonLimitsDataFitsBP);
        telescope->defineNumber(HorizonLimitsPointNP);
        telescope->defineSwitch(HorizonLimitsTraverseSP);
        telescope->defineSwitch(HorizonLimitsManageSP);
        telescope->defineSwitch(HorizonLimitsFileOperationSP);
        telescope->defineSwitch(HorizonLimitsOnLimitSP);
        telescope->defineSwitch(HorizonLimitsLimitGotoSP);
    }
}


bool HorizonLimits::updateProperties()
{
    //IDLog("HorizonLimits update properties connected = %d.\n",(telescope->isConnected()?1:0) );
    if (telescope->isConnected())
    {
        telescope->defineText(HorizonLimitsDataFileTP);
        telescope->defineBLOB(HorizonLimitsDataFitsBP);
        telescope->defineNumber(HorizonLimitsPointNP);
        telescope->defineSwitch(HorizonLimitsTraverseSP);
        telescope->defineSwitch(HorizonLimitsManageSP);
        telescope->defineSwitch(HorizonLimitsFileOperationSP);
        telescope->defineSwitch(HorizonLimitsOnLimitSP);
        telescope->defineSwitch(HorizonLimitsLimitGotoSP);
        Init();
    }
    else if (HorizonLimitsDataFileTP)
    {
        telescope->deleteProperty(HorizonLimitsDataFileTP->name);
        telescope->deleteProperty(HorizonLimitsDataFitsBP->name);
        telescope->deleteProperty(HorizonLimitsPointNP->name);
        telescope->deleteProperty(HorizonLimitsTraverseSP->name);
        telescope->deleteProperty(HorizonLimitsManageSP->name);
        telescope->deleteProperty(HorizonLimitsFileOperationSP->name);
        telescope->deleteProperty(HorizonLimitsOnLimitSP->name);
        telescope->deleteProperty(HorizonLimitsLimitGotoSP->name);
    }
    return true;
}

bool HorizonLimits::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    if (strcmp(dev, telescope->getDeviceName()) == 0)
    {
        if (HorizonLimitsPointNP && strcmp(name, HorizonLimitsPointNP->name) == 0)
        {
            horizonpoint hp;
            std::vector<horizonpoint>::iterator low;
            HorizonLimitsPointNP->s = IPS_OK;
            if (IUUpdateNumber(HorizonLimitsPointNP, values, names, n) != 0)
            {
                HorizonLimitsPointNP->s = IPS_ALERT;
                IDSetNumber(HorizonLimitsPointNP, NULL);
                return false;
            }
            hp.az  = values[0];
            hp.alt = values[1];
            if (binary_search(horizon->begin(), horizon->end(), hp, cmphorizonpoint))
            {
                DEBUGF(INDI::Logger::DBG_WARNING,
                       "Horizon Limits: point with Az = %f already present. Delete it first.", hp.az);
                HorizonLimitsPointNP->s = IPS_ALERT;
                IDSetNumber(HorizonLimitsPointNP, NULL);
                return false;
            }
            horizon->push_back(hp);
            std::sort(horizon->begin(), horizon->end(), cmphorizonpoint);
            low          = std::lower_bound(horizon->begin(), horizon->end(), hp, cmphorizonpoint);
            horizonindex = std::distance(horizon->begin(), low);
            DEBUGF(INDI::Logger::DBG_SESSION,
                   "Horizon Limits: Added point Az = %f, Alt  = %f, Rank=%d (Total %d points)", hp.az, hp.alt,
                   horizonindex, horizon->size());
            IDSetNumber(HorizonLimitsPointNP, NULL);
            return true;
        }
    }
    return false;
}

bool HorizonLimits::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, telescope->getDeviceName()) == 0)
    {
        if (HorizonLimitsTraverseSP && strcmp(name, HorizonLimitsTraverseSP->name) == 0)
        {
            if (!horizon || horizon->size() == 0)
            {
                DEBUG(INDI::Logger::DBG_WARNING, "Horizon Limits: Can not traverse empty list");
                HorizonLimitsTraverseSP->s = IPS_ALERT;
                IDSetSwitch(HorizonLimitsTraverseSP, NULL);
                return true;
            }
            ISwitch *sw;
            INumber *az  = IUFindNumber(HorizonLimitsPointNP, "HORIZONLIMITS_POINT_AZ");
            INumber *alt = IUFindNumber(HorizonLimitsPointNP, "HORIZONLIMITS_POINT_ALT");
            IUUpdateSwitch(HorizonLimitsTraverseSP, states, names, n);
            sw = IUFindOnSwitch(HorizonLimitsTraverseSP);
            if (!strcmp(sw->name, "HORIZONLIMITSLISTFIRST"))
            {
                horizonindex = 0;
            }
            if (!strcmp(sw->name, "HORIZONLIMITSLISTPREV"))
            {
                if (horizonindex > 0)
                    horizonindex = horizonindex - 1;
		else
		    if (horizonindex == -1)
		        horizonindex = horizon->size() - 1;
            }
            if (!strcmp(sw->name, "HORIZONLIMITSLISTNEXT"))
            {
	      if (horizonindex < (int)(horizon->size() - 1))
                    horizonindex = horizonindex + 1;
            }
            if (!strcmp(sw->name, "HORIZONLIMITSLISTLAST"))
            {
                horizonindex = horizon->size() - 1;
            }
            az->value               = horizon->at(horizonindex).az;
            alt->value              = horizon->at(horizonindex).alt;
            HorizonLimitsPointNP->s = IPS_OK;
            IDSetNumber(HorizonLimitsPointNP, NULL);
            HorizonLimitsTraverseSP->s = IPS_OK;
            IDSetSwitch(HorizonLimitsTraverseSP, NULL);
            return true;
        }

        if (HorizonLimitsManageSP && strcmp(name, HorizonLimitsManageSP->name) == 0)
        {
            ISwitch *sw;
            IUUpdateSwitch(HorizonLimitsManageSP, states, names, n);
            sw           = IUFindOnSwitch(HorizonLimitsManageSP);
            INumber *az  = IUFindNumber(HorizonLimitsPointNP, "HORIZONLIMITS_POINT_AZ");
            INumber *alt = IUFindNumber(HorizonLimitsPointNP, "HORIZONLIMITS_POINT_ALT");
            if (!strcmp(sw->name, "HORIZONLIMITSLISTADDCURRENT"))
            {
                horizonpoint hp;
                std::vector<horizonpoint>::iterator low;
                double values[2];
                const char *names[]                     = { "HORIZONLIMITS_POINT_AZ", "HORIZONLIMITS_POINT_ALT" };
                INumberVectorProperty *horizontalcoords = telescope->getNumber("HORIZONTAL_COORD");
                if (!horizontalcoords)
                {
                    DEBUG(INDI::Logger::DBG_WARNING, "Horizon Limits: Scope does not support horizontal coordinates.");
                    HorizonLimitsManageSP->s = IPS_ALERT;
                    IDSetSwitch(HorizonLimitsManageSP, NULL);
                    return false;
                }
                values[0] = IUFindNumber(horizontalcoords, "AZ")->value;
                values[1] = IUFindNumber(horizontalcoords, "ALT")->value;
                if (IUUpdateNumber(HorizonLimitsPointNP, values, (char **)names, 2) != 0)
                {
                    HorizonLimitsPointNP->s = IPS_ALERT;
                    IDSetNumber(HorizonLimitsPointNP, NULL);
                    HorizonLimitsManageSP->s = IPS_ALERT;
                    IDSetSwitch(HorizonLimitsManageSP, NULL);
                    return false;
                }
                hp.az  = values[0];
                hp.alt = values[1];
                if (binary_search(horizon->begin(), horizon->end(), hp, cmphorizonpoint))
                {
                    DEBUGF(INDI::Logger::DBG_WARNING,
                           "Horizon Limits: point with Az = %f already present. Delete it first.", hp.az);
                    HorizonLimitsManageSP->s = IPS_ALERT;
                    IDSetSwitch(HorizonLimitsManageSP, NULL);
                    return false;
                }
                horizon->push_back(hp);
                std::sort(horizon->begin(), horizon->end(), cmphorizonpoint);
                low          = std::lower_bound(horizon->begin(), horizon->end(), hp, cmphorizonpoint);
                horizonindex = std::distance(horizon->begin(), low);
                DEBUGF(INDI::Logger::DBG_SESSION,
                       "Horizon Limits: Added point Az = %f, Alt  = %f, Rank=%d (Total %d points)", hp.az, hp.alt,
                       horizonindex, horizon->size());
                HorizonLimitsPointNP->s = IPS_OK;
                IDSetNumber(HorizonLimitsPointNP, NULL);
                HorizonLimitsManageSP->s = IPS_OK;
                IDSetSwitch(HorizonLimitsManageSP, NULL);
                return true;
            }
            else if (!strcmp(sw->name, "HORIZONLIMITSLISTDELETE"))
            {
	      if (!horizon || (horizonindex >= (int)horizon->size()))
                {
                    DEBUG(INDI::Logger::DBG_WARNING, "Horizon Limits: Can not delete point");
                    HorizonLimitsManageSP->s = IPS_ALERT;
                    IDSetSwitch(HorizonLimitsManageSP, NULL);
                    return true;
                }
                DEBUGF(INDI::Logger::DBG_SESSION, "Horizon Limits: Deleted point Az = %f, Alt  = %f, Rank=%d",
                       horizon->at(horizonindex).az, horizon->at(horizonindex).alt, horizonindex);
                horizon->erase(horizon->begin() + horizonindex);
                if (horizonindex >= (int)horizon->size())
                    horizonindex = horizon->size() - 1;
                az->value               = horizon->at(horizonindex).az;
                alt->value              = horizon->at(horizonindex).alt;
                HorizonLimitsPointNP->s = IPS_OK;
                IDSetNumber(HorizonLimitsPointNP, NULL);
                HorizonLimitsManageSP->s = IPS_OK;
                IDSetSwitch(HorizonLimitsManageSP, NULL);
                return true;
            }
            else if (!strcmp(sw->name, "HORIZONLIMITSLISTCLEAR"))
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Horizon Limits: List cleared");
                if (horizon)
                    horizon->erase(horizon->begin(), horizon->end());
                horizonindex            = -1;
                az->value               = 0.0;
                alt->value              = 0.0;
                HorizonLimitsPointNP->s = IPS_OK;
                IDSetNumber(HorizonLimitsPointNP, NULL);
                HorizonLimitsManageSP->s = IPS_OK;
                IDSetSwitch(HorizonLimitsManageSP, NULL);
                return true;
            }
        }
        if (HorizonLimitsFileOperationSP && strcmp(name, HorizonLimitsFileOperationSP->name) == 0)
        {
            ISwitch *sw;
            IUUpdateSwitch(HorizonLimitsFileOperationSP, states, names, n);
            sw = IUFindOnSwitch(HorizonLimitsFileOperationSP);
            if (!strcmp(sw->name, "HORIZONLIMITSWRITEFILE"))
            {
                char *res;
                res = WriteDataFile(IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text);
                if (res)
                {
                    DEBUGF(INDI::Logger::DBG_WARNING, "Can not save HorizonLimits Data to file %s: %s",
                           IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text, res);
                    HorizonLimitsFileOperationSP->s = IPS_ALERT;
                }
                else
                {
                    DEBUGF(INDI::Logger::DBG_SESSION, "HorizonLimits: Data saved in file %s",
                           IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text);
                    HorizonLimitsFileOperationSP->s = IPS_OK;
                }
            }
            else if (!strcmp(sw->name, "HORIZONLIMITSLOADFILE"))
            {
                char *res;
                //Reset();
                res = LoadDataFile(IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text);
                if (res)
                {
                    DEBUGF(INDI::Logger::DBG_WARNING, "Can not load HorizonLimits Data File %s: %s",
                           IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text, res);
                    HorizonLimitsFileOperationSP->s = IPS_ALERT;
                }
                else
                {
                    DEBUGF(INDI::Logger::DBG_SESSION, "HorizonLimits: Data loaded from file %s",
                           IUFindText(HorizonLimitsDataFileTP, "HORIZONLIMITSFILENAME")->text);
                    HorizonLimitsFileOperationSP->s = IPS_OK;
                }
            }
            IDSetSwitch(HorizonLimitsFileOperationSP, NULL);
            return true;
        }

        if (HorizonLimitsOnLimitSP && strcmp(name, HorizonLimitsOnLimitSP->name) == 0)
        {
            HorizonLimitsOnLimitSP->s = IPS_OK;
            IUUpdateSwitch(HorizonLimitsOnLimitSP, states, names, n);
            IDSetSwitch(HorizonLimitsOnLimitSP, NULL);
            return true;
        }

        if (HorizonLimitsLimitGotoSP && strcmp(name, HorizonLimitsLimitGotoSP->name) == 0)
        {
            HorizonLimitsLimitGotoSP->s = IPS_OK;
            IUUpdateSwitch(HorizonLimitsLimitGotoSP, states, names, n);
            IDSetSwitch(HorizonLimitsLimitGotoSP, NULL);
            return true;
        }
    }

    return false;
}

bool HorizonLimits::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev, telescope->getDeviceName()) == 0)
    {
        if (HorizonLimitsDataFileTP && (strcmp(name, HorizonLimitsDataFileTP->name) == 0))
        {
            IUUpdateText(HorizonLimitsDataFileTP, texts, names, n);
            IDSetText(HorizonLimitsDataFileTP, NULL);
            return true;
        }
    }
    return false;
}

bool HorizonLimits::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                              char *formats[], char *names[], int num)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(num);
    return false;
}

char *HorizonLimits::WriteDataFile(const char *filename)
{
    wordexp_t wexp;
    FILE *fp;
    char lon[10], lat[10];
    INumberVectorProperty *geo = telescope->getNumber("GEOGRAPHIC_COORD");
    INumber *nlon              = IUFindNumber(geo, "LONG");
    INumber *nlat              = IUFindNumber(geo, "LAT");

    if (wordexp(filename, &wexp, 0))
    {
        wordfree(&wexp);
        return (char *)("Badly formed filename");
    }

    if (!horizon)
    {
        return (char *)"No Horizon data";
    }

    if (!(fp = fopen(wexp.we_wordv[0], "w")))
    {
        wordfree(&wexp);
        return strerror(errno);
    }

    setlocale(LC_NUMERIC, "C");

    numberFormat(lon, "%10.6m", nlon->value);
    numberFormat(lat, "%10.6m", nlat->value);
    fprintf(fp, "# Horizon Data for device %s\n", getDeviceName());
    fprintf(fp, "# Location: longitude=%s latitude=%s\n", lon, lat);
    fprintf(fp, "# Created on %s by %s\n", timestamp(), telescope->getDriverName());
    for (std::vector<horizonpoint>::iterator it = horizon->begin(); it != horizon->end(); ++it)
        fprintf(fp, "%g %g\n", it->az, it->alt);

    fclose(fp);
    setlocale(LC_NUMERIC, "");
    return NULL;
}

char *HorizonLimits::LoadDataFile(const char *filename)
{
    wordexp_t wexp;
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    horizonpoint hp;
    int nline = 0, pos = 0;
    INumber *az  = IUFindNumber(HorizonLimitsPointNP, "HORIZONLIMITS_POINT_AZ");
    INumber *alt = IUFindNumber(HorizonLimitsPointNP, "HORIZONLIMITS_POINT_ALT");

    if (wordexp(filename, &wexp, 0))
    {
        wordfree(&wexp);
        return (char *)("Badly formed filename");
    }

    if (!(fp = fopen(wexp.we_wordv[0], "r")))
    {
        wordfree(&wexp);
        return strerror(errno);
    }
    wordfree(&wexp);
    Reset();
    setlocale(LC_NUMERIC, "C");

    while ((read = getline(&line, &len, fp)) != -1)
    {
        char *s = line;
        while ((*s == ' ') || (*s == '\t'))
            s++;
        if (*s == '#')
            continue;
        if (sscanf(s, "%lg%n", &hp.az, &pos) != 1)
        {
            fclose(fp);
            snprintf((char *)sline, 4, "%d", nline);
            setlocale(LC_NUMERIC, "");
            return (char *)errorline;
        }
        s += pos;
        while ((*s == ' ') || (*s == '\t'))
            s++;
        if (sscanf(s, "%lg%n", &hp.alt, &pos) != 1)
        {
            fclose(fp);
            snprintf((char *)sline, 4, "%d", nline);
            setlocale(LC_NUMERIC, "");
            return (char *)errorline;
        }
        horizon->push_back(hp);
        nline++;
        pos = 0;
    }

    horizonindex            = -1;
    az->value               = 0.0;
    alt->value              = 0.0;
    HorizonLimitsPointNP->s = IPS_OK;
    IDSetNumber(HorizonLimitsPointNP, NULL);

    if (line)
        free(line);

    fclose(fp);
    setlocale(LC_NUMERIC, "");
    return NULL;
}

bool HorizonLimits::inLimits(double az, double alt)
{
    horizonpoint scope;
    double a, b, h;
    if ((!horizon) || (horizon->size() == 0))
        return alt >= 0.0;
    scope.az  = az;
    scope.alt = alt;
    std::vector<horizonpoint>::iterator low;
    std::vector<horizonpoint>::iterator prev;
    low = std::lower_bound(horizon->begin(), horizon->end(), scope, cmphorizonpoint);
    if (low->az == az)
        return (alt >= low->alt);
    if (low != horizon->begin())
        prev = low - 1;
    else
        prev = horizon->end();
    if (low != prev)
    {
        a = (low->alt - prev->alt) / (low->az - prev->az);
        b = prev->alt - (a * prev->az);
        h = (a * az) + b;
        return ((alt - h) >= 0.0);
    }
    else
    {
        return ((alt - low->alt) >= 0.0);
    }
}

bool HorizonLimits::inGotoLimits(double az, double alt)
{
    ISwitch *swlimitgotodisable = IUFindSwitch(HorizonLimitsLimitGotoSP, "HORIZONLIMITSLIMITGOTODISABLE");
    return (inLimits(az, alt) || (swlimitgotodisable->s == ISS_ON));
}

bool HorizonLimits::checkLimits(double az, double alt, INDI::Telescope::TelescopeStatus status, bool ingoto)
{
    bool abortscope = false;
    const char *abortmsg;
    ISwitch *swaborttrack = IUFindSwitch(HorizonLimitsOnLimitSP, "HORIZONLIMITSONLIMITTRACK");
    ISwitch *swabortslew  = IUFindSwitch(HorizonLimitsOnLimitSP, "HORIZONLIMITSONLIMITSLEW");
    ISwitch *swabortgoto  = IUFindSwitch(HorizonLimitsOnLimitSP, "HORIZONLIMITSONLIMITGOTO");
    if (!(inLimits(az, alt)))
    {
        abortmsg = "Nothing to abort.";
        if ((status == INDI::Telescope::SCOPE_TRACKING) && (swaborttrack->s == ISS_ON))
        {
            abortmsg   = "Abort Tracking.";
            abortscope = true;
        }
        if ((status == INDI::Telescope::SCOPE_SLEWING) && (swabortslew->s == ISS_ON) && !ingoto)
        {
            abortmsg   = "Abort Slewing.";
            abortscope = true;
        }
        if ((status == INDI::Telescope::SCOPE_SLEWING) && (swabortgoto->s == ISS_ON) && ingoto)
        {
            abortmsg   = "Abort Goto.";
            abortscope = true;
        }
        DEBUGF(INDI::Logger::DBG_WARNING, "Horizon Limits: Scope outside limits. %s", abortmsg);
    }
    return (abortscope);
}
