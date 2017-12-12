/*!
 * \file AlignmentSubsystemForClients.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "AlignmentSubsystemForClients.h"

namespace INDI
{
namespace AlignmentSubsystem
{
void AlignmentSubsystemForClients::Initialise(const char *DeviceName, INDI::BaseClient *BaseClient)
{
    AlignmentSubsystemForClients::DeviceName = DeviceName;
    ClientAPIForAlignmentDatabase::Initialise(BaseClient);
    ClientAPIForMathPluginManagement::Initialise(BaseClient);
}

void AlignmentSubsystemForClients::ProcessNewBLOB(IBLOB *BLOBPointer)
{
    if (strcmp(BLOBPointer->bvp->device, DeviceName.c_str()) == 0)
    {
        IDLog("newBLOB %s\n", BLOBPointer->bvp->name);
        ClientAPIForAlignmentDatabase::ProcessNewBLOB(BLOBPointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewDevice(INDI::BaseDevice *DevicePointer)
{
    if (strcmp(DevicePointer->getDeviceName(), DeviceName.c_str()) == 0)
    {
        IDLog("Receiving %s Device...\n", DevicePointer->getDeviceName());
        ClientAPIForAlignmentDatabase::ProcessNewDevice(DevicePointer);
        ClientAPIForMathPluginManagement::ProcessNewDevice(DevicePointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewNumber(INumberVectorProperty *NumberVectorPropertyPointer)
{
    if (strcmp(NumberVectorPropertyPointer->device, DeviceName.c_str()) == 0)
    {
        IDLog("newNumber %s\n", NumberVectorPropertyPointer->name);
        ClientAPIForAlignmentDatabase::ProcessNewNumber(NumberVectorPropertyPointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewProperty(INDI::Property *PropertyPointer)
{
    if (strcmp(PropertyPointer->getDeviceName(), DeviceName.c_str()) == 0)
    {
        IDLog("newProperty %s\n", PropertyPointer->getName());
        ClientAPIForAlignmentDatabase::ProcessNewProperty(PropertyPointer);
        ClientAPIForMathPluginManagement::ProcessNewProperty(PropertyPointer);
    }
}

void AlignmentSubsystemForClients::ProcessNewSwitch(ISwitchVectorProperty *SwitchVectorPropertyPointer)
{
    if (strcmp(SwitchVectorPropertyPointer->device, DeviceName.c_str()) == 0)
    {
        IDLog("newSwitch %s\n", SwitchVectorPropertyPointer->name);
        ClientAPIForAlignmentDatabase::ProcessNewSwitch(SwitchVectorPropertyPointer);
        ClientAPIForMathPluginManagement::ProcessNewSwitch(SwitchVectorPropertyPointer);
    }
}

} // namespace AlignmentSubsystem
} // namespace INDI
