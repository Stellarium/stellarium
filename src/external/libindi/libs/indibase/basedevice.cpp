/*******************************************************************************
  Copyright(c) 2011 Jasem Mutlaq. All rights reserved.

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

#include "basedevice.h"

#include "base64.h"
#include "config.h"
#include "indicom.h"
#include "indistandardproperty.h"
#include "locale_compat.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <zlib.h>
#include <sys/stat.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#pragma warning(push)
///@todo Introduce platform independent safe functions as macros to fix this
#pragma warning(disable : 4996)
#endif

namespace INDI
{

BaseDevice::BaseDevice()
{
    mediator = nullptr;
    lp       = newLilXML();
    deviceID = new char[MAXINDIDEVICE];
    memset(deviceID, 0, MAXINDIDEVICE);

    char indidev[MAXINDIDEVICE];
    strncpy(indidev, "INDIDEV=", MAXINDIDEVICE);

    if (getenv("INDIDEV") != nullptr)
    {
        strncpy(deviceID, getenv("INDIDEV"), MAXINDIDEVICE);
        putenv(indidev);
    }
}

BaseDevice::~BaseDevice()
{
    delLilXML(lp);
    while (!pAll.empty())
    {
        delete pAll.back();
        pAll.pop_back();
    }
    messageLog.clear();

    delete[] deviceID;
}

INumberVectorProperty *BaseDevice::getNumber(const char *name)
{
    return static_cast<INumberVectorProperty *>(getRawProperty(name, INDI_NUMBER));
}

ITextVectorProperty *BaseDevice::getText(const char *name)
{
    return static_cast<ITextVectorProperty *>(getRawProperty(name, INDI_TEXT));
}

ISwitchVectorProperty *BaseDevice::getSwitch(const char *name)
{
    return static_cast<ISwitchVectorProperty *>(getRawProperty(name, INDI_SWITCH));
}

ILightVectorProperty *BaseDevice::getLight(const char *name)
{
    return static_cast<ILightVectorProperty *>(getRawProperty(name, INDI_LIGHT));
}

IBLOBVectorProperty *BaseDevice::getBLOB(const char *name)
{
    return static_cast<IBLOBVectorProperty *>(getRawProperty(name, INDI_BLOB));
}

IPState BaseDevice::getPropertyState(const char *name)
{
    IPState state = IPS_IDLE;
    INumberVectorProperty *nvp;
    ITextVectorProperty *tvp;
    ISwitchVectorProperty *svp;
    ILightVectorProperty *lvp;
    IBLOBVectorProperty *bvp;

    std::vector<INDI::Property *>::iterator orderi = pAll.begin();

    for (; orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr  = (*orderi)->getProperty();

        switch (pType)
        {
            case INDI_NUMBER:
                nvp = static_cast<INumberVectorProperty *>(pPtr);
                if (nvp == nullptr)
                    continue;
                if (!strcmp(name, nvp->name))
                    return nvp->s;
                break;
            case INDI_SWITCH:
                svp = static_cast<ISwitchVectorProperty *>(pPtr);
                if (svp == nullptr)
                    continue;
                if (!strcmp(name, svp->name))
                    return svp->s;
                break;
            case INDI_TEXT:
                tvp = static_cast<ITextVectorProperty *>(pPtr);
                if (tvp == nullptr)
                    continue;
                if (!strcmp(name, tvp->name))
                    return tvp->s;
                break;
            case INDI_LIGHT:
                lvp = static_cast<ILightVectorProperty *>(pPtr);
                if (lvp == nullptr)
                    continue;
                if (!strcmp(name, lvp->name))
                    return lvp->s;
                break;
            case INDI_BLOB:
                bvp = static_cast<IBLOBVectorProperty *>(pPtr);
                if (bvp == nullptr)
                    continue;
                if (!strcmp(name, bvp->name))
                    return bvp->s;
                break;
            default:
                break;
        }
    }

    return state;
}

IPerm BaseDevice::getPropertyPermission(const char *name)
{
    IPerm perm = IP_RO;
    INumberVectorProperty *nvp;
    ITextVectorProperty *tvp;
    ISwitchVectorProperty *svp;
    IBLOBVectorProperty *bvp;

    std::vector<INDI::Property *>::iterator orderi = pAll.begin();

    for (; orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr  = (*orderi)->getProperty();

        switch (pType)
        {
            case INDI_NUMBER:
                nvp = static_cast<INumberVectorProperty *>(pPtr);
                if (nvp == nullptr)
                    continue;
                if (!strcmp(name, nvp->name))
                    return nvp->p;
                break;
            case INDI_SWITCH:
                svp = static_cast<ISwitchVectorProperty *>(pPtr);
                if (svp == nullptr)
                    continue;
                if (!strcmp(name, svp->name))
                    return svp->p;
                break;
            case INDI_TEXT:
                tvp = static_cast<ITextVectorProperty *>(pPtr);
                if (tvp == nullptr)
                    continue;
                if (!strcmp(name, tvp->name))
                    return tvp->p;
                break;
            case INDI_BLOB:
                bvp = static_cast<IBLOBVectorProperty *>(pPtr);
                if (bvp == nullptr)
                    continue;
                if (!strcmp(name, bvp->name))
                    return bvp->p;
                break;
            default:
                break;
        }
    }

    return perm;
}

void *BaseDevice::getRawProperty(const char *name, INDI_PROPERTY_TYPE type)
{
    std::vector<INDI::Property *>::iterator orderi = pAll.begin();

    INumberVectorProperty *nvp = nullptr;
    ITextVectorProperty *tvp = nullptr;
    ISwitchVectorProperty *svp = nullptr;
    ILightVectorProperty *lvp = nullptr;
    IBLOBVectorProperty *bvp = nullptr;

    for (; orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr = (*orderi)->getProperty();
        bool pRegistered = (*orderi)->getRegistered();

        if (type != INDI_UNKNOWN && pType != type)
            continue;

        switch (pType)
        {
            case INDI_NUMBER:
                nvp = static_cast<INumberVectorProperty *>(pPtr);
                if (nvp == nullptr)
                    continue;

                if (!strcmp(name, nvp->name) && pRegistered)
                    return pPtr;
                break;
            case INDI_TEXT:
                tvp = static_cast<ITextVectorProperty *>(pPtr);
                if (tvp == nullptr)
                    continue;

                if (!strcmp(name, tvp->name) && pRegistered)
                    return pPtr;
                break;
            case INDI_SWITCH:
                svp = static_cast<ISwitchVectorProperty *>(pPtr);
                if (svp == nullptr)
                    continue;

                //IDLog("Switch %s and aux value is now %d\n", svp->name, regStatus );
                if (!strcmp(name, svp->name) && pRegistered)
                    return pPtr;
                break;
            case INDI_LIGHT:
                lvp = static_cast<ILightVectorProperty *>(pPtr);
                if (lvp == nullptr)
                    continue;

                if (!strcmp(name, lvp->name) && pRegistered)
                    return pPtr;
                break;
            case INDI_BLOB:
                bvp = static_cast<IBLOBVectorProperty *>(pPtr);
                if (bvp == nullptr)
                    continue;

                if (!strcmp(name, bvp->name) && pRegistered)
                    return pPtr;
                break;

            case INDI_UNKNOWN:
                return nullptr;
        }
    }

    return nullptr;
}

INDI::Property *BaseDevice::getProperty(const char *name, INDI_PROPERTY_TYPE type)
{
    std::vector<INDI::Property *>::iterator orderi;

    INumberVectorProperty *nvp;
    ITextVectorProperty *tvp;
    ISwitchVectorProperty *svp;
    ILightVectorProperty *lvp;
    IBLOBVectorProperty *bvp;

    for (orderi = pAll.begin(); orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr = (*orderi)->getProperty();
        bool pRegistered = (*orderi)->getRegistered();

        if (type != INDI_UNKNOWN && pType != type)
            continue;

        switch (pType)
        {
            case INDI_NUMBER:
                nvp = static_cast<INumberVectorProperty *>(pPtr);
                if (nvp == nullptr)
                    continue;

                if (!strcmp(name, nvp->name) && pRegistered)
                    return *orderi;
                break;
            case INDI_TEXT:
                tvp = static_cast<ITextVectorProperty *>(pPtr);
                if (tvp == nullptr)
                    continue;

                if (!strcmp(name, tvp->name) && pRegistered)
                    return *orderi;
                break;
            case INDI_SWITCH:
                svp = static_cast<ISwitchVectorProperty *>(pPtr);
                if (svp == nullptr)
                    continue;

                //IDLog("Switch %s and aux value is now %d\n", svp->name, regStatus );
                if (!strcmp(name, svp->name) && pRegistered)
                    return *orderi;
                break;
            case INDI_LIGHT:
                lvp = static_cast<ILightVectorProperty *>(pPtr);
                if (lvp == nullptr)
                    continue;

                if (!strcmp(name, lvp->name) && pRegistered)
                    return *orderi;
                break;
            case INDI_BLOB:
                bvp = static_cast<IBLOBVectorProperty *>(pPtr);
                if (bvp == nullptr)
                    continue;

                if (!strcmp(name, bvp->name) && pRegistered)
                    return *orderi;
                break;
            case INDI_UNKNOWN:
                break;
        }
    }

    return nullptr;
}

int BaseDevice::removeProperty(const char *name, char *errmsg)
{
    std::vector<INDI::Property *>::iterator orderi;

    INumberVectorProperty *nvp;
    ITextVectorProperty *tvp;
    ISwitchVectorProperty *svp;
    ILightVectorProperty *lvp;
    IBLOBVectorProperty *bvp;

    for (orderi = pAll.begin(); orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr  = (*orderi)->getProperty();

        switch (pType)
        {
            case INDI_NUMBER:
                nvp = static_cast<INumberVectorProperty *>(pPtr);
                if (!strcmp(name, nvp->name))
                {
                    (*orderi)->setRegistered(false);
                    delete *orderi;
                    orderi = pAll.erase(orderi);

                    return 0;
                }
                break;
            case INDI_TEXT:
                tvp = static_cast<ITextVectorProperty *>(pPtr);
                if (!strcmp(name, tvp->name))
                {
                    (*orderi)->setRegistered(false);
                    delete *orderi;
                    orderi = pAll.erase(orderi);

                    return 0;
                }
                break;
            case INDI_SWITCH:
                svp = static_cast<ISwitchVectorProperty *>(pPtr);
                if (!strcmp(name, svp->name))
                {
                    (*orderi)->setRegistered(false);
                    delete *orderi;
                    orderi = pAll.erase(orderi);
                    return 0;
                }
                break;
            case INDI_LIGHT:
                lvp = static_cast<ILightVectorProperty *>(pPtr);
                if (!strcmp(name, lvp->name))
                {
                    (*orderi)->setRegistered(false);
                    delete *orderi;
                    orderi = pAll.erase(orderi);
                    return 0;
                }
                break;
            case INDI_BLOB:
                bvp = static_cast<IBLOBVectorProperty *>(pPtr);
                if (!strcmp(name, bvp->name))
                {
                    (*orderi)->setRegistered(false);
                    delete *orderi;
                    orderi = pAll.erase(orderi);
                    return 0;
                }
                break;
            case INDI_UNKNOWN:
                break;
        }
    }

    snprintf(errmsg, MAXRBUF, "Error: Property %s not found in device %s.", name, deviceID);
    return INDI_PROPERTY_INVALID;
}

bool BaseDevice::buildSkeleton(const char *filename)
{
    char errmsg[MAXRBUF];
    FILE *fp     = nullptr;
    XMLEle *root = nullptr, *fproot = nullptr;

    char pathname[MAXRBUF];
    struct stat st;
    const char *indiskel = getenv("INDISKEL");
    if (indiskel)
    {
        strncpy(pathname, indiskel, MAXRBUF - 1);
        pathname[MAXRBUF - 1] = 0;
        IDLog("Using INDISKEL %s\n", pathname);
    }
    else
    {
        if (stat(filename, &st) == 0)
        {
            strncpy(pathname, filename, MAXRBUF - 1);
            pathname[MAXRBUF - 1] = 0;
            IDLog("Using %s\n", pathname);
        }
        else
        {
            const char *slash = strrchr(filename, '/');
            if (slash)
                filename = slash + 1;
            const char *indiprefix = getenv("INDIPREFIX");
            if (indiprefix)
            {
#if defined(OSX_EMBEDED_MODE)
                snprintf(pathname, MAXRBUF - 1, "%s/Contents/Resources/%s", indiprefix, filename);
#elif defined(__APPLE__)
                snprintf(pathname, MAXRBUF - 1, "%s/Contents/Resources/DriverSupport/%s", indiprefix, filename);
#else
                snprintf(pathname, MAXRBUF - 1, "%s/share/indi/%s", indiprefix, filename);
#endif
            }
            else
            {
                snprintf(pathname, MAXRBUF - 1, "%s/%s", DATA_INSTALL_DIR, filename);
            }
            pathname[MAXRBUF - 1] = 0;
            IDLog("Using prefix %s\n", pathname);
        }
    }

    fp = fopen(pathname, "r");

    if (fp == nullptr)
    {
        IDLog("Unable to build skeleton. Error loading file %s: %s\n", pathname, strerror(errno));
        return false;
    }

    fproot = readXMLFile(fp, lp, errmsg);
    fclose(fp);

    if (fproot == nullptr)
    {
        IDLog("Unable to parse skeleton XML: %s", errmsg);
        return false;
    }

    //prXMLEle(stderr, fproot, 0);

    for (root = nextXMLEle(fproot, 1); root != nullptr; root = nextXMLEle(fproot, 0))
        buildProp(root, errmsg);

    delXMLEle(fproot);
    return true;
    /**************************************************************************/
}

int BaseDevice::buildProp(XMLEle *root, char *errmsg)
{
    IPerm perm    = IP_RO;
    IPState state = IPS_IDLE;
    XMLEle *ep    = nullptr;
    char *rtag, *rname, *rdev;
    double timeout = 0;

    rtag = tagXMLEle(root);

    /* pull out device and name */
    if (crackDN(root, &rdev, &rname, errmsg) < 0)
        return -1;

    if (!deviceID[0])
        strncpy(deviceID, rdev, MAXINDINAME);

    //if (getProperty(rname, type) != nullptr)
    if (getProperty(rname) != nullptr)
        return INDI_PROPERTY_DUPLICATED;

    if (strcmp(rtag, "defLightVector") && crackIPerm(findXMLAttValu(root, "perm"), &perm) < 0)
    {
        IDLog("Error extracting %s permission (%s)\n", rname, findXMLAttValu(root, "perm"));
        return -1;
    }

    timeout = atoi(findXMLAttValu(root, "timeout"));

    if (crackIPState(findXMLAttValu(root, "state"), &state) < 0)
    {
        IDLog("Error extracting %s state (%s)\n", rname, findXMLAttValu(root, "state"));
        return -1;
    }

    if (!strcmp(rtag, "defNumberVector"))
    {
        AutoCNumeric locale;

        INDI::Property *indiProp   = new INDI::Property();
        INumberVectorProperty *nvp = new INumberVectorProperty;

        INumber *np = nullptr;
        int n       = 0;

        strncpy(nvp->device, deviceID, MAXINDIDEVICE);
        strncpy(nvp->name, rname, MAXINDINAME);
        strncpy(nvp->label, findXMLAttValu(root, "label"), MAXINDILABEL);
        strncpy(nvp->group, findXMLAttValu(root, "group"), MAXINDIGROUP);

        nvp->p       = perm;
        nvp->s       = state;
        nvp->timeout = timeout;

        /* pull out each name/value pair */
        for (n = 0, ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0), n++)
        {
            if (!strcmp(tagXMLEle(ep), "defNumber"))
            {
                np = static_cast<INumber *>(realloc(np, (n + 1) * sizeof(INumber)));

                np[n].nvp = nvp;

                XMLAtt *na = findXMLAtt(ep, "name");

                if (na)
                {
                    if (f_scansexa(pcdataXMLEle(ep), &(np[n].value)) < 0)
                        IDLog("%s: Bad format %s\n", rname, pcdataXMLEle(ep));
                    else
                    {
                        strncpy(np[n].name, valuXMLAtt(na), MAXINDINAME);
                        np[n].aux0 = nullptr;
                        np[n].aux1 = nullptr;

                        na = findXMLAtt(ep, "label");
                        if (na)
                            strncpy(np[n].label, valuXMLAtt(na), MAXINDILABEL);
                        na = findXMLAtt(ep, "format");
                        if (na)
                            strncpy(np[n].format, valuXMLAtt(na), MAXINDIFORMAT);

                        na = findXMLAtt(ep, "min");
                        if (na)
                            np[n].min = atof(valuXMLAtt(na));
                        na = findXMLAtt(ep, "max");
                        if (na)
                            np[n].max = atof(valuXMLAtt(na));
                        na = findXMLAtt(ep, "step");
                        if (na)
                            np[n].step = atof(valuXMLAtt(na));
                    }
                }
            }
        }

        if (n > 0)
        {
            nvp->nnp = n;
            nvp->np  = np;

            indiProp->setBaseDevice(this);
            indiProp->setProperty(nvp);
            indiProp->setDynamic(true);
            indiProp->setType(INDI_NUMBER);

            pAll.push_back(indiProp);

            //IDLog("Adding number property %s to list.\n", nvp->name);
            if (mediator)
                mediator->newProperty(indiProp);
        }
        else
        {
            IDLog("%s: newNumberVector with no valid members\n", rname);
            delete (nvp);
            delete (indiProp);
        }
    }
    else if (!strcmp(rtag, "defSwitchVector"))
    {
        INDI::Property *indiProp   = new INDI::Property();
        ISwitchVectorProperty *svp = new ISwitchVectorProperty;

        ISwitch *sp = nullptr;
        int n       = 0;

        strncpy(svp->device, deviceID, MAXINDIDEVICE);
        strncpy(svp->name, rname, MAXINDINAME);
        strncpy(svp->label, findXMLAttValu(root, "label"), MAXINDILABEL);
        strncpy(svp->group, findXMLAttValu(root, "group"), MAXINDIGROUP);

        if (crackISRule(findXMLAttValu(root, "rule"), (&svp->r)) < 0)
            svp->r = ISR_1OFMANY;

        svp->p       = perm;
        svp->s       = state;
        svp->timeout = timeout;

        /* pull out each name/value pair */
        for (n = 0, ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0), n++)
        {
            if (!strcmp(tagXMLEle(ep), "defSwitch"))
            {
                sp = static_cast<ISwitch *>(realloc(sp, (n + 1) * sizeof(ISwitch)));

                sp[n].svp = svp;

                XMLAtt *na = findXMLAtt(ep, "name");

                if (na)
                {
                    crackISState(pcdataXMLEle(ep), &(sp[n].s));
                    strncpy(sp[n].name, valuXMLAtt(na), MAXINDINAME);
                    sp[n].aux = nullptr;

                    na = findXMLAtt(ep, "label");
                    if (na)
                        strncpy(sp[n].label, valuXMLAtt(na), MAXINDILABEL);
                }
            }
        }

        if (n > 0)
        {
            svp->nsp = n;
            svp->sp  = sp;

            indiProp->setBaseDevice(this);
            indiProp->setProperty(svp);
            indiProp->setDynamic(true);
            indiProp->setType(INDI_SWITCH);

            pAll.push_back(indiProp);
            //IDLog("Adding Switch property %s to list.\n", svp->name);
            if (mediator)
                mediator->newProperty(indiProp);
        }
        else
        {
            IDLog("%s: newSwitchVector with no valid members\n", rname);
            delete (svp);
            delete (indiProp);
        }
    }

    else if (!strcmp(rtag, "defTextVector"))
    {
        INDI::Property *indiProp = new INDI::Property();
        ITextVectorProperty *tvp = new ITextVectorProperty;
        IText *tp                = nullptr;
        int n                    = 0;

        strncpy(tvp->device, deviceID, MAXINDIDEVICE);
        strncpy(tvp->name, rname, MAXINDINAME);
        strncpy(tvp->label, findXMLAttValu(root, "label"), MAXINDILABEL);
        strncpy(tvp->group, findXMLAttValu(root, "group"), MAXINDIGROUP);

        tvp->p       = perm;
        tvp->s       = state;
        tvp->timeout = timeout;

        // pull out each name/value pair
        for (n = 0, ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0), n++)
        {
            if (!strcmp(tagXMLEle(ep), "defText"))
            {
                tp = static_cast<IText *>(realloc(tp, (n + 1) * sizeof(IText)));

                tp[n].tvp = tvp;

                XMLAtt *na = findXMLAtt(ep, "name");

                if (na)
                {
                    tp[n].text = static_cast<char *>(malloc((pcdatalenXMLEle(ep) * sizeof(char)) + 1));
                    strncpy(tp[n].text, pcdataXMLEle(ep), pcdatalenXMLEle(ep));
                    tp[n].text[pcdatalenXMLEle(ep)] = '\0';
                    tp[n].aux0 = nullptr;
                    tp[n].aux1 = nullptr;
                    strncpy(tp[n].name, valuXMLAtt(na), MAXINDINAME);

                    na = findXMLAtt(ep, "label");
                    if (na)
                        strncpy(tp[n].label, valuXMLAtt(na), MAXINDILABEL);
                }
            }
        }

        if (n > 0)
        {
            tvp->ntp = n;
            tvp->tp  = tp;

            indiProp->setBaseDevice(this);
            indiProp->setProperty(tvp);
            indiProp->setDynamic(true);
            indiProp->setType(INDI_TEXT);

            pAll.push_back(indiProp);

            //IDLog("Adding Text property %s to list with initial value of %s.\n", tvp->name, tvp->tp[0].text);
            if (mediator)
                mediator->newProperty(indiProp);
        }
        else
        {
            IDLog("%s: newTextVector with no valid members\n", rname);
            delete (tvp);
            delete (indiProp);
        }
    }
    else if (!strcmp(rtag, "defLightVector"))
    {
        INDI::Property *indiProp  = new INDI::Property();
        ILightVectorProperty *lvp = new ILightVectorProperty;
        ILight *lp                = nullptr;
        int n                     = 0;

        strncpy(lvp->device, deviceID, MAXINDIDEVICE);
        strncpy(lvp->name, rname, MAXINDINAME);
        strncpy(lvp->label, findXMLAttValu(root, "label"), MAXINDILABEL);
        strncpy(lvp->group, findXMLAttValu(root, "group"), MAXINDIGROUP);

        lvp->s = state;

        /* pull out each name/value pair */
        for (n = 0, ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0), n++)
        {
            if (!strcmp(tagXMLEle(ep), "defLight"))
            {
                lp = static_cast<ILight *>(realloc(lp, (n + 1) * sizeof(ILight)));

                lp[n].lvp = lvp;

                XMLAtt *na = findXMLAtt(ep, "name");

                if (na)
                {
                    crackIPState(pcdataXMLEle(ep), &(lp[n].s));
                    strncpy(lp[n].name, valuXMLAtt(na), MAXINDINAME);
                    lp[n].aux = nullptr;

                    na = findXMLAtt(ep, "label");
                    if (na)
                        strncpy(lp[n].label, valuXMLAtt(na), MAXINDILABEL);
                }
            }
        }

        if (n > 0)
        {
            lvp->nlp = n;
            lvp->lp  = lp;

            indiProp->setBaseDevice(this);
            indiProp->setProperty(lvp);
            indiProp->setDynamic(true);
            indiProp->setType(INDI_LIGHT);

            pAll.push_back(indiProp);

            //IDLog("Adding Light property %s to list.\n", lvp->name);
            if (mediator)
                mediator->newProperty(indiProp);
        }
        else
        {
            IDLog("%s: newLightVector with no valid members\n", rname);
            delete (lvp);
            delete (indiProp);
        }
    }
    else if (!strcmp(rtag, "defBLOBVector"))
    {
        INDI::Property *indiProp = new INDI::Property();
        IBLOBVectorProperty *bvp = new IBLOBVectorProperty;
        IBLOB *bp                = nullptr;
        int n                    = 0;

        strncpy(bvp->device, deviceID, MAXINDIDEVICE);
        strncpy(bvp->name, rname, MAXINDINAME);
        strncpy(bvp->label, findXMLAttValu(root, "label"), MAXINDILABEL);
        strncpy(bvp->group, findXMLAttValu(root, "group"), MAXINDIGROUP);

        bvp->s = state;
        bvp->p = perm;
        bvp->timeout = timeout;

        /* pull out each name/value pair */
        for (n = 0, ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0), n++)
        {
            if (!strcmp(tagXMLEle(ep), "defBLOB"))
            {
                bp = static_cast<IBLOB *>(realloc(bp, (n + 1) * sizeof(IBLOB)));

                bp[n].bvp = bvp;

                XMLAtt *na = findXMLAtt(ep, "name");

                if (na)
                {
                    strncpy(bp[n].name, valuXMLAtt(na), MAXINDINAME);

                    na = findXMLAtt(ep, "label");
                    if (na)
                        strncpy(bp[n].label, valuXMLAtt(na), MAXINDILABEL);

                    na = findXMLAtt(ep, "format");
                    if (na)
                        strncpy(bp[n].label, valuXMLAtt(na), MAXINDIBLOBFMT);

                    // Initialize everything to zero

                    // Seed for realloc
                    bp[n].blob    = nullptr;
                    bp[n].size    = 0;
                    bp[n].bloblen = 0;
                    bp[n].aux0    = nullptr;
                    bp[n].aux1    = nullptr;
                    bp[n].aux2    = nullptr;
                }
            }
        }

        if (n > 0)
        {
            bvp->nbp = n;
            bvp->bp  = bp;

            indiProp->setBaseDevice(this);
            indiProp->setProperty(bvp);
            indiProp->setDynamic(true);
            indiProp->setType(INDI_BLOB);

            pAll.push_back(indiProp);
            //IDLog("Adding BLOB property %s to list.\n", bvp->name);
            if (mediator)
                mediator->newProperty(indiProp);
        }
        else
        {
            IDLog("%s: newBLOBVector with no valid members\n", rname);
            delete (bvp);
            delete (indiProp);
        }
    }

    return (0);
}

bool BaseDevice::isConnected()
{
    ISwitchVectorProperty *svp = getSwitch(INDI::SP::CONNECTION);
    if (!svp)
        return false;

    ISwitch *sp = IUFindSwitch(svp, "CONNECT");

    if (!sp)
        return false;

    if (sp->s == ISS_ON && svp->s == IPS_OK)
        return true;
    else
        return false;
}

/*
 * return 0 if ok else -1 with reason in errmsg
 */
int BaseDevice::setValue(XMLEle *root, char *errmsg)
{
    XMLEle *ep = nullptr;
    char *name = nullptr;
    double timeout = 0;
    IPState state = IPS_IDLE;
    bool stateSet = false, timeoutSet = false;

    char *rtag = tagXMLEle(root);

    XMLAtt *ap = findXMLAtt(root, "name");
    if (!ap)
    {
        snprintf(errmsg, MAXRBUF, "INDI: <%s> unable to find name attribute", tagXMLEle(root));
        return (-1);
    }

    name = valuXMLAtt(ap);

    /* set overall property state, if any */
    ap = findXMLAtt(root, "state");
    if (ap)
    {
        if (crackIPState(valuXMLAtt(ap), &state) != 0)
        {
            snprintf(errmsg, MAXRBUF, "INDI: <%s> bogus state %s for %s", tagXMLEle(root), valuXMLAtt(ap), name);
            return (-1);
        }

        stateSet = true;
    }

    /* allow changing the timeout */
    ap = findXMLAtt(root, "timeout");
    if (ap)
    {
        AutoCNumeric locale;
        timeout    = atof(valuXMLAtt(ap));
        timeoutSet = true;
    }

    checkMessage(root);

    if (!strcmp(rtag, "setNumberVector"))
    {
        INumberVectorProperty *nvp = getNumber(name);
        if (nvp == nullptr)
        {
            snprintf(errmsg, MAXRBUF, "INDI: Could not find property %s in %s", name, deviceID);
            return -1;
        }

        if (stateSet)
            nvp->s = state;

        if (timeoutSet)
            nvp->timeout = timeout;

        AutoCNumeric locale;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            INumber *np = IUFindNumber(nvp, findXMLAttValu(ep, "name"));
            if (!np)
                continue;

            np->value = atof(pcdataXMLEle(ep));

            // Permit changing of min/max
            if (findXMLAtt(ep, "min"))
                np->min = atof(findXMLAttValu(ep, "min"));
            if (findXMLAtt(ep, "max"))
                np->max = atof(findXMLAttValu(ep, "max"));
        }

        locale.Restore();

        if (mediator)
            mediator->newNumber(nvp);

        return 0;
    }
    else if (!strcmp(rtag, "setTextVector"))
    {
        ITextVectorProperty *tvp = getText(name);
        if (tvp == nullptr)
            return -1;

        if (stateSet)
            tvp->s = state;

        if (timeoutSet)
            tvp->timeout = timeout;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            IText *tp = IUFindText(tvp, findXMLAttValu(ep, "name"));
            if (!tp)
                continue;

            IUSaveText(tp, pcdataXMLEle(ep));
        }

        if (mediator)
            mediator->newText(tvp);

        return 0;
    }
    else if (!strcmp(rtag, "setSwitchVector"))
    {
        ISState swState;
        ISwitchVectorProperty *svp = getSwitch(name);
        if (svp == nullptr)
            return -1;

        if (stateSet)
            svp->s = state;

        if (timeoutSet)
            svp->timeout = timeout;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            ISwitch *sp = IUFindSwitch(svp, findXMLAttValu(ep, "name"));
            if (!sp)
                continue;

            if (crackISState(pcdataXMLEle(ep), &swState) == 0)
                sp->s = swState;
        }

        if (mediator)
            mediator->newSwitch(svp);

        return 0;
    }
    else if (!strcmp(rtag, "setLightVector"))
    {
        IPState lState;
        ILightVectorProperty *lvp = getLight(name);

        if (lvp == nullptr)
            return -1;

        if (stateSet)
            lvp->s = state;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            ILight *lp = IUFindLight(lvp, findXMLAttValu(ep, "name"));
            if (!lp)
                continue;

            if (crackIPState(pcdataXMLEle(ep), &lState) == 0)
                lp->s = lState;
        }

        if (mediator)
            mediator->newLight(lvp);

        return 0;
    }
    else if (!strcmp(rtag, "setBLOBVector"))
    {
        IBLOBVectorProperty *bvp = getBLOB(name);

        if (bvp == nullptr)
            return -1;

        if (stateSet)
            bvp->s = state;

        if (timeoutSet)
            bvp->timeout = timeout;

        return setBLOB(bvp, root, errmsg);
    }

    snprintf(errmsg, MAXRBUF, "INDI: <%s> Unable to process tag", tagXMLEle(root));
    return -1;
}

/* Set BLOB vector. Process incoming data stream
 * Return 0 if okay, -1 if error
*/
int BaseDevice::setBLOB(IBLOBVectorProperty *bvp, XMLEle *root, char *errmsg)
{
    /* pull out each name/BLOB pair, decode */
    for (XMLEle *ep = nextXMLEle(root, 1); ep; ep = nextXMLEle(root, 0))
    {
        if (strcmp(tagXMLEle(ep), "oneBLOB") == 0)
        {
            XMLAtt *na = findXMLAtt(ep, "name");

            IBLOB *blobEL = IUFindBLOB(bvp, findXMLAttValu(ep, "name"));

            XMLAtt *fa = findXMLAtt(ep, "format");
            XMLAtt *sa = findXMLAtt(ep, "size");
            if (na && fa && sa)
            {
                int blobSize = atoi(valuXMLAtt(sa));

                /* Blob size = 0 when only state changes */
                if (blobSize == 0)
                {
                    if (mediator)
                        mediator->newBLOB(blobEL);
                    continue;
                }

                blobEL->size    = blobSize;
                int bloblen     = pcdatalenXMLEle(ep);
                int blobBufferSize = 3 * bloblen / 4;
                if (blobBufferSize != blobEL->bloblen)
                    blobEL->blob    = static_cast<unsigned char *>(realloc(blobEL->blob, blobBufferSize));
                blobEL->bloblen = from64tobits_fast(static_cast<char *>(blobEL->blob), pcdataXMLEle(ep), bloblen);

                strncpy(blobEL->format, valuXMLAtt(fa), MAXINDIFORMAT);

                if (strstr(blobEL->format, ".z"))
                {
                    blobEL->format[strlen(blobEL->format) - 2] = '\0';
                    uLongf dataSize = blobEL->size * sizeof(uint8_t);
                    uint8_t *dataBuffer = static_cast<uint8_t *>(malloc(dataSize));

                    if (dataBuffer == nullptr)
                    {
                        strncpy(errmsg, "Unable to allocate memory for data buffer", MAXRBUF);
                        return (-1);
                    }

                    int r = uncompress(dataBuffer, &dataSize, static_cast<unsigned char *>(blobEL->blob),
                                       static_cast<uLong>(blobEL->bloblen));
                    if (r != Z_OK)
                    {
                        snprintf(errmsg, MAXRBUF, "INDI: %s.%s.%s compression error: %d", blobEL->bvp->device,
                                 blobEL->bvp->name, blobEL->name, r);
                        free(dataBuffer);
                        return -1;
                    }
                    blobEL->size = dataSize;
                    free(blobEL->blob);
                    blobEL->blob = dataBuffer;
                }

                if (mediator)
                    mediator->newBLOB(blobEL);
            }
            else
            {
                snprintf(errmsg, MAXRBUF, "INDI: %s.%s.%s No valid members.", blobEL->bvp->device, blobEL->bvp->name,
                         blobEL->name);
                return -1;
            }
        }
    }

    return 0;
}

void BaseDevice::setDeviceName(const char *dev)
{
    strncpy(deviceID, dev, MAXINDINAME);
}

const char *BaseDevice::getDeviceName()
{
    return deviceID;
}

/* add message to queue
 * N.B. don't put carriage control in msg, we take care of that.
 */
void BaseDevice::checkMessage(XMLEle *root)
{
    XMLAtt *ap;
    ap = findXMLAtt(root, "message");

    if (ap)
        doMessage(root);
}

/* Store msg in queue */
void BaseDevice::doMessage(XMLEle *msg)
{
    XMLAtt *message;
    XMLAtt *time_stamp;

    char msgBuffer[MAXRBUF];

    /* prefix our timestamp if not with msg */
    time_stamp = findXMLAtt(msg, "timestamp");

    /* finally! the msg */
    message = findXMLAtt(msg, "message");
    if (!message)
        return;

    if (time_stamp)
        snprintf(msgBuffer, MAXRBUF, "%s: %s ", valuXMLAtt(time_stamp), valuXMLAtt(message));
    else
        snprintf(msgBuffer, MAXRBUF, "%s: %s ", timestamp(), valuXMLAtt(message));

    std::string finalMsg = msgBuffer;

    // Prepend to the log
    addMessage(finalMsg);
}

void BaseDevice::addMessage(const std::string &msg)
{
    messageLog.push_back(msg);

    if (mediator)
        mediator->newMessage(this, messageLog.size() - 1);
}

std::string BaseDevice::messageQueue(int index) const
{
    if (index >= static_cast<int>(messageLog.size()))
        return nullptr;

    return messageLog.at(index);
}

std::string BaseDevice::lastMessage()
{
    return messageLog.back();
}

void BaseDevice::registerProperty(void *p, INDI_PROPERTY_TYPE type)
{
    INDI::Property *pContainer;

    if (type == INDI_NUMBER)
    {
        INumberVectorProperty *nvp = static_cast<INumberVectorProperty *>(p);
        if ((pContainer = getProperty(nvp->name, INDI_NUMBER)) != nullptr)
        {
            pContainer->setRegistered(true);
            return;
        }

        pContainer = new INDI::Property();
        pContainer->setProperty(p);
        pContainer->setType(type);

        pAll.push_back(pContainer);
    }
    else if (type == INDI_TEXT)
    {
        ITextVectorProperty *tvp = static_cast<ITextVectorProperty *>(p);

        if ((pContainer = getProperty(tvp->name, INDI_TEXT)) != nullptr)
        {
            pContainer->setRegistered(true);
            return;
        }

        pContainer = new INDI::Property();
        pContainer->setProperty(p);
        pContainer->setType(type);

        pAll.push_back(pContainer);
    }
    else if (type == INDI_SWITCH)
    {
        ISwitchVectorProperty *svp = static_cast<ISwitchVectorProperty *>(p);

        if ((pContainer = getProperty(svp->name, INDI_SWITCH)) != nullptr)
        {
            pContainer->setRegistered(true);
            return;
        }

        pContainer = new INDI::Property();
        pContainer->setProperty(p);
        pContainer->setType(type);

        pAll.push_back(pContainer);
    }
    else if (type == INDI_LIGHT)
    {
        ILightVectorProperty *lvp = static_cast<ILightVectorProperty *>(p);

        if ((pContainer = getProperty(lvp->name, INDI_LIGHT)) != nullptr)
        {
            pContainer->setRegistered(true);
            return;
        }

        pContainer = new INDI::Property();
        pContainer->setProperty(p);
        pContainer->setType(type);

        pAll.push_back(pContainer);
    }
    else if (type == INDI_BLOB)
    {
        IBLOBVectorProperty *bvp = static_cast<IBLOBVectorProperty *>(p);

        if ((pContainer = getProperty(bvp->name, INDI_BLOB)) != nullptr)
        {
            pContainer->setRegistered(true);
            return;
        }

        pContainer = new INDI::Property();
        pContainer->setProperty(p);
        pContainer->setType(type);

        pAll.push_back(pContainer);
    }
}

const char *BaseDevice::getDriverName()
{
    ITextVectorProperty *driverInfo = getText("DRIVER_INFO");

    if (driverInfo == nullptr)
        return nullptr;

    IText *driverName = IUFindText(driverInfo, "DRIVER_NAME");
    if (driverName)
        return driverName->text;

    return nullptr;
}

const char *BaseDevice::getDriverExec()
{
    ITextVectorProperty *driverInfo = getText("DRIVER_INFO");

    if (driverInfo == nullptr)
        return nullptr;

    IText *driverExec = IUFindText(driverInfo, "DRIVER_EXEC");
    if (driverExec)
        return driverExec->text;

    return nullptr;
}

const char *BaseDevice::getDriverVersion()
{
    ITextVectorProperty *driverInfo = getText("DRIVER_INFO");

    if (driverInfo == nullptr)
        return nullptr;

    IText *driverVersion = IUFindText(driverInfo, "DRIVER_VERSION");
    if (driverVersion)
        return driverVersion->text;

    return nullptr;
}

uint16_t BaseDevice::getDriverInterface()
{
    ITextVectorProperty *driverInfo = getText("DRIVER_INFO");

    if (driverInfo == nullptr)
        return 0;

    IText *driverInterface = IUFindText(driverInfo, "DRIVER_INTERFACE");
    if (driverInterface)
        return atoi(driverInterface->text);

    return 0;
}

}

#if defined(_MSC_VER)
#undef snprintf
#pragma warning(pop)
#endif
