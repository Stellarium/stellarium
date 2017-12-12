/*!
 * \file AlignmentSubsystemForDrivers.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "AlignmentSubsystemForDrivers.h"

namespace INDI
{
namespace AlignmentSubsystem
{
AlignmentSubsystemForDrivers::AlignmentSubsystemForDrivers()
{
    // Set up the in memory database pointer for math plugins
    SetCurrentInMemoryDatabase(this);
    // Tell the built in math plugin about it
    Initialise(this);
    // Fix up the database load callback
    SetLoadDatabaseCallback(&MyDatabaseLoadCallback, this);
}

// Public methods

void AlignmentSubsystemForDrivers::InitAlignmentProperties(Telescope *pTelescope)
{
    MapPropertiesToInMemoryDatabase::InitProperties(pTelescope);
    MathPluginManagement::InitProperties(pTelescope);
}

void AlignmentSubsystemForDrivers::ProcessAlignmentBLOBProperties(Telescope *pTelescope, const char *name, int sizes[],
                                                                  int blobsizes[], char *blobs[], char *formats[],
                                                                  char *names[], int n)
{
    MapPropertiesToInMemoryDatabase::ProcessBlobProperties(pTelescope, name, sizes, blobsizes, blobs, formats, names,
                                                           n);
}

void AlignmentSubsystemForDrivers::ProcessAlignmentNumberProperties(Telescope *pTelescope, const char *name,
                                                                    double values[], char *names[], int n)
{
    MapPropertiesToInMemoryDatabase::ProcessNumberProperties(pTelescope, name, values, names, n);
}

void AlignmentSubsystemForDrivers::ProcessAlignmentSwitchProperties(Telescope *pTelescope, const char *name,
                                                                    ISState *states, char *names[], int n)
{
    MapPropertiesToInMemoryDatabase::ProcessSwitchProperties(pTelescope, name, states, names, n);
    MathPluginManagement::ProcessSwitchProperties(pTelescope, name, states, names, n);
}

void AlignmentSubsystemForDrivers::ProcessAlignmentTextProperties(Telescope *pTelescope, const char *name,
                                                                  char *texts[], char *names[], int n)
{
    MathPluginManagement::ProcessTextProperties(pTelescope, name, texts, names, n);
}

void AlignmentSubsystemForDrivers::SaveAlignmentConfigProperties(FILE *fp)
{
    MathPluginManagement::SaveConfigProperties(fp);
}

// Private methods

void AlignmentSubsystemForDrivers::MyDatabaseLoadCallback(void *ThisPointer)
{
    ((AlignmentSubsystemForDrivers *)ThisPointer)->Initialise((AlignmentSubsystemForDrivers *)ThisPointer);
}

} // namespace AlignmentSubsystem
} // namespace INDI
