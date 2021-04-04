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

#include "indiproperty.h"

#include <cstdlib>

INDI::Property::Property()
{
    pPtr        = nullptr;
    pRegistered = false;
    pDynamic    = false;
    pType       = INDI_UNKNOWN;
}

INDI::Property::~Property()
{
    // Only delete properties if they were created dynamically via the buildSkeleton
    // function. Other drivers are responsible for their own memory allocation.
    if (pDynamic)
    {
        switch (pType)
        {
            case INDI_NUMBER:
            {
                INumberVectorProperty *p = static_cast<INumberVectorProperty *>(pPtr);
                free(p->np);
                delete p;
                break;
            }

            case INDI_TEXT:
            {
                ITextVectorProperty *p = static_cast<ITextVectorProperty *>(pPtr);
                for (int i = 0; i < p->ntp; ++i)
                {
                    free(p->tp[i].text);
                }
                free(p->tp);
                delete p;
                break;
            }

            case INDI_SWITCH:
            {
                ISwitchVectorProperty *p = static_cast<ISwitchVectorProperty *>(pPtr);
                free(p->sp);
                delete p;
                break;
            }

            case INDI_LIGHT:
            {
                ILightVectorProperty *p = static_cast<ILightVectorProperty *>(pPtr);
                free(p->lp);
                delete p;
                break;
            }

            case INDI_BLOB:
            {
                IBLOBVectorProperty *p = static_cast<IBLOBVectorProperty *>(pPtr);
                for (int i = 0; i < p->nbp; ++i)
                {
                    free(p->bp[i].blob);
                }
                free(p->bp);
                delete p;
                break;
            }

            case INDI_UNKNOWN:
                break;
        }
    }
}

void INDI::Property::setProperty(void *p)
{
    pRegistered = true;
    pPtr        = p;
}

void INDI::Property::setType(INDI_PROPERTY_TYPE t)
{
    pType = t;
}

void INDI::Property::setRegistered(bool r)
{
    pRegistered = r;
}

void INDI::Property::setDynamic(bool d)
{
    pDynamic = d;
}

void INDI::Property::setBaseDevice(BaseDevice *idp)
{
    dp = idp;
}

const char *INDI::Property::getName() const
{
    if (pPtr == nullptr)
        return nullptr;

    switch (pType)
    {
        case INDI_NUMBER:
            return (static_cast<INumberVectorProperty *>(pPtr)->name);
        case INDI_TEXT:
            return (static_cast<ITextVectorProperty *>(pPtr)->name);
        case INDI_SWITCH:
            return (static_cast<ISwitchVectorProperty *>(pPtr)->name);
        case INDI_LIGHT:
            return (static_cast<ILightVectorProperty *>(pPtr)->name);
        case INDI_BLOB:
            return (static_cast<IBLOBVectorProperty *>(pPtr)->name);
        default:
            return nullptr;
    }
}

const char *INDI::Property::getLabel() const
{
    if (pPtr == nullptr)
        return nullptr;

    switch (pType)
    {
        case INDI_NUMBER:
            return static_cast <INumberVectorProperty * >(pPtr)->label;
        case INDI_TEXT:
            return static_cast <ITextVectorProperty * >(pPtr)->label;
        case INDI_SWITCH:
            return static_cast <ISwitchVectorProperty * >(pPtr)->label;
        case INDI_LIGHT:
            return static_cast <ILightVectorProperty * >(pPtr)->label;
        case INDI_BLOB:
            return static_cast <IBLOBVectorProperty * >(pPtr)->label;
        default:
            return nullptr;
    }
}


const char *INDI::Property::getGroupName() const
{
    if (pPtr == nullptr)
        return nullptr;

    switch (pType)
    {
        case INDI_NUMBER:
            return static_cast <INumberVectorProperty * >(pPtr)->group;
        case INDI_TEXT:
            return static_cast <ITextVectorProperty * >(pPtr)->group;
        case INDI_SWITCH:
            return static_cast <ISwitchVectorProperty * >(pPtr)->group;
        case INDI_LIGHT:
            return static_cast <ILightVectorProperty * >(pPtr)->group;
        case INDI_BLOB:
            return static_cast <IBLOBVectorProperty * >(pPtr)->group;
        default:
            return nullptr;
    }
}

const char *INDI::Property::getDeviceName() const
{
    if (pPtr == nullptr)
        return nullptr;

    switch (pType)
    {
        case INDI_NUMBER:
            return static_cast <INumberVectorProperty * >(pPtr)->device;
        case INDI_TEXT:
            return static_cast <ITextVectorProperty * >(pPtr)->device;
        case INDI_SWITCH:
            return static_cast <ISwitchVectorProperty * >(pPtr)->device;
        case INDI_LIGHT:
            return static_cast <ILightVectorProperty * >(pPtr)->device;
        case INDI_BLOB:
            return static_cast <IBLOBVectorProperty * >(pPtr)->device;
        default:
            return nullptr;
    }
}


const char *INDI::Property::getTimestamp() const
{
    switch (pType)
    {
        case INDI_NUMBER:
            return static_cast <INumberVectorProperty * >(pPtr)->timestamp;
        case INDI_TEXT:
            return static_cast <ITextVectorProperty * >(pPtr)->timestamp;
        case INDI_SWITCH:
            return static_cast <ISwitchVectorProperty * >(pPtr)->timestamp;
        case INDI_LIGHT:
            return static_cast <ILightVectorProperty * >(pPtr)->timestamp;
        case INDI_BLOB:
            return static_cast <IBLOBVectorProperty * >(pPtr)->timestamp;
        default:
            return nullptr;
    }
}

IPState INDI::Property::getState() const
{
    switch (pType)
    {
        case INDI_NUMBER:
            return static_cast <INumberVectorProperty * >(pPtr)->s;
        case INDI_TEXT:
            return static_cast <ITextVectorProperty * >(pPtr)->s;
        case INDI_SWITCH:
            return static_cast <ISwitchVectorProperty * >(pPtr)->s;
        case INDI_LIGHT:
            return static_cast <ILightVectorProperty * >(pPtr)->s;
        case INDI_BLOB:
            return static_cast <IBLOBVectorProperty * >(pPtr)->s;
        default:
            return IPS_ALERT;
    }
}

IPerm INDI::Property::getPermission() const
{
    switch (pType)
    {
        case INDI_NUMBER:
            return static_cast <INumberVectorProperty * >(pPtr)->p;
        case INDI_TEXT:
            return static_cast <ITextVectorProperty * >(pPtr)->p;
        case INDI_SWITCH:
            return static_cast <ISwitchVectorProperty * >(pPtr)->p;
        case INDI_BLOB:
            return static_cast <IBLOBVectorProperty * >(pPtr)->p;
        default:
            return IP_RO;
    }
}

INumberVectorProperty *INDI::Property::getNumber() const
{
    if (pType == INDI_NUMBER)
        return static_cast <INumberVectorProperty * > (pPtr);

    return nullptr;
}

ITextVectorProperty *INDI::Property::getText() const
{
    if (pType == INDI_TEXT)
        return static_cast <ITextVectorProperty * > (pPtr);

    return nullptr;
}

ILightVectorProperty *INDI::Property::getLight() const
{
    if (pType == INDI_LIGHT)
        return static_cast <ILightVectorProperty * > (pPtr);

    return nullptr;
}

ISwitchVectorProperty *INDI::Property::getSwitch() const
{
    if (pType == INDI_SWITCH)
        return static_cast <ISwitchVectorProperty * > (pPtr);

    return nullptr;
}

IBLOBVectorProperty *INDI::Property::getBLOB() const
{
    if (pType == INDI_BLOB)
        return static_cast <IBLOBVectorProperty * > (pPtr);

    return nullptr;
}
