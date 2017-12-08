/*!
 * \file ClientAPIForAlignmentDatabase.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "ClientAPIForAlignmentDatabase.h"

#include "indicom.h"

namespace INDI
{
namespace AlignmentSubsystem
{
ClientAPIForAlignmentDatabase::ClientAPIForAlignmentDatabase()
{
    pthread_cond_init(&DriverActionCompleteCondition, nullptr);
    pthread_mutex_init(&DriverActionCompleteMutex, nullptr);
}

ClientAPIForAlignmentDatabase::~ClientAPIForAlignmentDatabase()
{
    pthread_cond_destroy(&DriverActionCompleteCondition);
    pthread_mutex_destroy(&DriverActionCompleteMutex);
}

bool ClientAPIForAlignmentDatabase::AppendSyncPoint(const AlignmentDatabaseEntry &CurrentValues)
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction = Action->getSwitch();
    ISwitchVectorProperty *pCommit = Commit->getSwitch();

    if (APPEND != IUFindOnSwitchIndex(pAction))
    {
        // Request Append mode
        IUResetSwitch(pAction);
        pAction->sp[APPEND].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("AppendSyncPoint - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    if (!SendEntryData(CurrentValues))
        return false;

    // Commit the entry to the database
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("AppendSyncPoint - Bad Commit switch state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

bool ClientAPIForAlignmentDatabase::ClearSyncPoints()
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction = Action->getSwitch();
    ISwitchVectorProperty *pCommit = Commit->getSwitch();

    // Select the required action
    if (CLEAR != IUFindOnSwitchIndex(pAction))
    {
        // Request Clear mode
        IUResetSwitch(pAction);
        pAction->sp[CLEAR].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("ClearSyncPoints - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("ClearSyncPoints - Bad Commit switch state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

bool ClientAPIForAlignmentDatabase::DeleteSyncPoint(unsigned int Offset)
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction       = Action->getSwitch();
    INumberVectorProperty *pCurrentEntry = CurrentEntry->getNumber();
    ISwitchVectorProperty *pCommit       = Commit->getSwitch();

    // Select the required action
    if (DELETE != IUFindOnSwitchIndex(pAction))
    {
        // Request Delete mode
        IUResetSwitch(pAction);
        pAction->sp[DELETE].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("DeleteSyncPoint - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Send the offset
    pCurrentEntry->np[0].value = Offset;
    SetDriverBusy();
    BaseClient->sendNewNumber(pCurrentEntry);
    WaitForDriverCompletion();
    if (IPS_OK != pCurrentEntry->s)
    {
        IDLog("DeleteSyncPoint - Bad Current Entry state %s\n", pstateStr(pCurrentEntry->s));
        return false;
    }

    // Commit the entry to the database
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("DeleteSyncPoint - Bad Commit switch state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

bool ClientAPIForAlignmentDatabase::EditSyncPoint(unsigned int Offset, const AlignmentDatabaseEntry &CurrentValues)
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction           = Action->getSwitch();
    INumberVectorProperty *pCurrentEntry     = CurrentEntry->getNumber();
    ISwitchVectorProperty *pCommit           = Commit->getSwitch();

    // Select the required action
    if (EDIT != IUFindOnSwitchIndex(pAction))
    {
        // Request Edit mode
        IUResetSwitch(pAction);
        pAction->sp[EDIT].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("EditSyncPoint - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Send the offset
    pCurrentEntry->np[0].value = Offset;
    SetDriverBusy();
    BaseClient->sendNewNumber(pCurrentEntry);
    WaitForDriverCompletion();
    if (IPS_OK != pCurrentEntry->s)
    {
        IDLog("EditSyncPoint - Bad Current Entry state %s\n", pstateStr(pCurrentEntry->s));
        return false;
    }

    if (!SendEntryData(CurrentValues))
        return false;

    // Commit the entry to the database
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("EditSyncPoint - Bad Commit switch state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

int ClientAPIForAlignmentDatabase::GetDatabaseSize()
{
    return 0;
}

void ClientAPIForAlignmentDatabase::Initialise(INDI::BaseClient *BaseClient)
{
    ClientAPIForAlignmentDatabase::BaseClient = BaseClient;
}

bool ClientAPIForAlignmentDatabase::InsertSyncPoint(unsigned int Offset, const AlignmentDatabaseEntry &CurrentValues)
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction           = Action->getSwitch();
    INumberVectorProperty *pCurrentEntry     = CurrentEntry->getNumber();
    ISwitchVectorProperty *pCommit           = Commit->getSwitch();

    // Select the required action
    if (INSERT != IUFindOnSwitchIndex(pAction))
    {
        // Request Insert mode
        IUResetSwitch(pAction);
        pAction->sp[INSERT].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("InsertSyncPoint - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Send the offset
    pCurrentEntry->np[0].value = Offset;
    SetDriverBusy();
    BaseClient->sendNewNumber(pCurrentEntry);
    WaitForDriverCompletion();
    if (IPS_OK != pCurrentEntry->s)
    {
        IDLog("InsertSyncPoint - Bad Current Entry state %s\n", pstateStr(pCurrentEntry->s));
        return false;
    }

    if (!SendEntryData(CurrentValues))
        return false;

    // Commit the entry to the database
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("InsertSyncPoint - Bad Commit switch state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

bool ClientAPIForAlignmentDatabase::LoadDatabase()
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction = Action->getSwitch();
    ISwitchVectorProperty *pCommit = Commit->getSwitch();

    // Select the required action
    if (LOAD_DATABASE != IUFindOnSwitchIndex(pAction))
    {
        // Request Load Database mode
        IUResetSwitch(pAction);
        pAction->sp[LOAD_DATABASE].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("LoadDatabase - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Commit the Load Database
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("LoadDatabase - Bad Commit state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

void ClientAPIForAlignmentDatabase::ProcessNewBLOB(IBLOB *BLOBPointer)
{
    if (strcmp(BLOBPointer->bvp->name, "ALIGNMENT_POINT_OPTIONAL_BINARY_BLOB") == 0)
    {
        if (IPS_BUSY != BLOBPointer->bvp->s)
        {
            ISwitchVectorProperty *pAction = Action->getSwitch();
            int Index                      = IUFindOnSwitchIndex(pAction);
            if ((READ != Index) && (READ_INCREMENT != Index))
                SignalDriverCompletion();
        }
    }
}

void ClientAPIForAlignmentDatabase::ProcessNewDevice(INDI::BaseDevice *DevicePointer)
{
    Device = DevicePointer;
}

void ClientAPIForAlignmentDatabase::ProcessNewNumber(INumberVectorProperty *NumberVectorProperty)
{
    if (strcmp(NumberVectorProperty->name, "ALIGNMENT_POINT_MANDATORY_NUMBERS") == 0)
    {
        if (IPS_BUSY != NumberVectorProperty->s)
        {
            ISwitchVectorProperty *pAction = Action->getSwitch();
            int Index                      = IUFindOnSwitchIndex(pAction);
            if ((READ != Index) && (READ_INCREMENT != Index))
                SignalDriverCompletion();
        }
    }
    else if (strcmp(NumberVectorProperty->name, "ALIGNMENT_POINTSET_CURRENT_ENTRY") == 0)
    {
        if (IPS_BUSY != NumberVectorProperty->s)
        {
            ISwitchVectorProperty *pAction = Action->getSwitch();
            int Index                      = IUFindOnSwitchIndex(pAction);
            if (READ_INCREMENT != Index)
                SignalDriverCompletion();
        }
    }
}

void ClientAPIForAlignmentDatabase::ProcessNewProperty(INDI::Property *PropertyPointer)
{
    bool GotOneOfMine = true;

    if (strcmp(PropertyPointer->getName(), "ALIGNMENT_POINT_MANDATORY_NUMBERS") == 0)
        MandatoryNumbers = PropertyPointer;
    else if (strcmp(PropertyPointer->getName(), "ALIGNMENT_POINT_OPTIONAL_BINARY_BLOB") == 0)
    {
        OptionalBinaryBlob = PropertyPointer;
        // Make sure the format string is set up
        strncpy(OptionalBinaryBlob->getBLOB()->bp->format, "alignmentPrivateData", MAXINDIBLOBFMT);
    }
    else if (strcmp(PropertyPointer->getName(), "ALIGNMENT_POINTSET_SIZE") == 0)
        PointsetSize = PropertyPointer;
    else if (strcmp(PropertyPointer->getName(), "ALIGNMENT_POINTSET_CURRENT_ENTRY") == 0)
        CurrentEntry = PropertyPointer;
    else if (strcmp(PropertyPointer->getName(), "ALIGNMENT_POINTSET_ACTION") == 0)
        Action = PropertyPointer;
    else if (strcmp(PropertyPointer->getName(), "ALIGNMENT_POINTSET_COMMIT") == 0)
        Commit = PropertyPointer;
    else
        GotOneOfMine = false;

    // Tell the client when all the database proeprties have been set up
    if (GotOneOfMine && (nullptr != MandatoryNumbers) && (nullptr != OptionalBinaryBlob) && (nullptr != PointsetSize) &&
        (nullptr != CurrentEntry) && (nullptr != Action) && (nullptr != Commit))
    {
        // The DriverActionComplete state variable is initialised to false
        // So I need to call this to set it to true and signal anyone
        // waiting for the driver to initialise etc.
        SignalDriverCompletion();
    }
}

void ClientAPIForAlignmentDatabase::ProcessNewSwitch(ISwitchVectorProperty *SwitchVectorProperty)
{
    if (strcmp(SwitchVectorProperty->name, "ALIGNMENT_POINTSET_ACTION") == 0)
    {
        if (IPS_BUSY != SwitchVectorProperty->s)
            SignalDriverCompletion();
    }
    else if (strcmp(SwitchVectorProperty->name, "ALIGNMENT_POINTSET_COMMIT") == 0)
    {
        if (IPS_BUSY != SwitchVectorProperty->s)
            SignalDriverCompletion();
    }
}

bool ClientAPIForAlignmentDatabase::ReadIncrementSyncPoint(AlignmentDatabaseEntry &CurrentValues)
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction           = Action->getSwitch();
    INumberVectorProperty *pMandatoryNumbers = MandatoryNumbers->getNumber();
    IBLOBVectorProperty *pBLOB               = OptionalBinaryBlob->getBLOB();
    INumberVectorProperty *pCurrentEntry     = CurrentEntry->getNumber();
    ISwitchVectorProperty *pCommit           = Commit->getSwitch();

    // Select the required action
    if (READ_INCREMENT != IUFindOnSwitchIndex(pAction))
    {
        // Request Read Increment mode
        IUResetSwitch(pAction);
        pAction->sp[READ_INCREMENT].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("ReadIncrementSyncPoint - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Commit the read increment
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if ((IPS_OK != pCommit->s) || (IPS_OK != pMandatoryNumbers->s) || (IPS_OK != pBLOB->s) ||
        (IPS_OK != pCurrentEntry->s))
    {
        IDLog("ReadIncrementSyncPoint - Bad Commit/Mandatory numbers/Blob/Current entry state %s %s %s %s\n",
              pstateStr(pCommit->s), pstateStr(pMandatoryNumbers->s), pstateStr(pBLOB->s), pstateStr(pCurrentEntry->s));
        return false;
    }

    // Read the entry data
    CurrentValues.ObservationJulianDate = pMandatoryNumbers->np[ENTRY_OBSERVATION_JULIAN_DATE].value;
    CurrentValues.RightAscension        = pMandatoryNumbers->np[ENTRY_RA].value;
    CurrentValues.Declination           = pMandatoryNumbers->np[ENTRY_DEC].value;
    CurrentValues.TelescopeDirection.x  = pMandatoryNumbers->np[ENTRY_VECTOR_X].value;
    CurrentValues.TelescopeDirection.y  = pMandatoryNumbers->np[ENTRY_VECTOR_Y].value;
    CurrentValues.TelescopeDirection.z  = pMandatoryNumbers->np[ENTRY_VECTOR_Z].value;

    return true;
}

bool ClientAPIForAlignmentDatabase::ReadSyncPoint(unsigned int Offset, AlignmentDatabaseEntry &CurrentValues)
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction           = Action->getSwitch();
    INumberVectorProperty *pMandatoryNumbers = MandatoryNumbers->getNumber();
    IBLOBVectorProperty *pBLOB               = OptionalBinaryBlob->getBLOB();
    INumberVectorProperty *pCurrentEntry     = CurrentEntry->getNumber();
    ISwitchVectorProperty *pCommit           = Commit->getSwitch();

    // Select the required action
    if (READ != IUFindOnSwitchIndex(pAction))
    {
        // Request Read mode
        IUResetSwitch(pAction);
        pAction->sp[READ].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("ReadSyncPoint - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Send the offset
    pCurrentEntry->np[0].value = Offset;
    SetDriverBusy();
    BaseClient->sendNewNumber(pCurrentEntry);
    WaitForDriverCompletion();
    if (IPS_OK != pCurrentEntry->s)
    {
        IDLog("ReadSyncPoint - Bad Current Entry state %s\n", pstateStr(pCurrentEntry->s));
        return false;
    }

    // Commit the read
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if ((IPS_OK != pCommit->s) || (IPS_OK != pMandatoryNumbers->s) || (IPS_OK != pBLOB->s))
    {
        IDLog("ReadSyncPoint - Bad Commit/Mandatory numbers/Blob state %s %s %s\n", pstateStr(pCommit->s),
              pstateStr(pMandatoryNumbers->s), pstateStr(pBLOB->s));
        return false;
    }

    // Read the entry data
    CurrentValues.ObservationJulianDate = pMandatoryNumbers->np[ENTRY_OBSERVATION_JULIAN_DATE].value;
    CurrentValues.RightAscension        = pMandatoryNumbers->np[ENTRY_RA].value;
    CurrentValues.Declination           = pMandatoryNumbers->np[ENTRY_DEC].value;
    CurrentValues.TelescopeDirection.x  = pMandatoryNumbers->np[ENTRY_VECTOR_X].value;
    CurrentValues.TelescopeDirection.y  = pMandatoryNumbers->np[ENTRY_VECTOR_Y].value;
    CurrentValues.TelescopeDirection.z  = pMandatoryNumbers->np[ENTRY_VECTOR_Z].value;

    return true;
}

bool ClientAPIForAlignmentDatabase::SaveDatabase()
{
    // Wait for driver to initialise if neccessary
    WaitForDriverCompletion();

    ISwitchVectorProperty *pAction = Action->getSwitch();
    ISwitchVectorProperty *pCommit = Commit->getSwitch();

    // Select the required action
    if (SAVE_DATABASE != IUFindOnSwitchIndex(pAction))
    {
        // Request Load Database mode
        IUResetSwitch(pAction);
        pAction->sp[SAVE_DATABASE].s = ISS_ON;
        SetDriverBusy();
        BaseClient->sendNewSwitch(pAction);
        WaitForDriverCompletion();
        if (IPS_OK != pAction->s)
        {
            IDLog("SaveDatabase - Bad Action switch state %s\n", pstateStr(pAction->s));
            return false;
        }
    }

    // Commit the Save Database
    IUResetSwitch(pCommit);
    pCommit->sp[0].s = ISS_ON;
    SetDriverBusy();
    BaseClient->sendNewSwitch(pCommit);
    WaitForDriverCompletion();
    if (IPS_OK != pCommit->s)
    {
        IDLog("Save Database - Bad Commit state %s\n", pstateStr(pCommit->s));
        return false;
    }

    return true;
}

// Private methods

bool ClientAPIForAlignmentDatabase::SendEntryData(const AlignmentDatabaseEntry &CurrentValues)
{
    INumberVectorProperty *pMandatoryNumbers = MandatoryNumbers->getNumber();
    IBLOBVectorProperty *pBLOB               = OptionalBinaryBlob->getBLOB();
    // Send the entry data
    pMandatoryNumbers->np[ENTRY_OBSERVATION_JULIAN_DATE].value = CurrentValues.ObservationJulianDate;
    pMandatoryNumbers->np[ENTRY_RA].value                      = CurrentValues.RightAscension;
    pMandatoryNumbers->np[ENTRY_DEC].value                     = CurrentValues.Declination;
    pMandatoryNumbers->np[ENTRY_VECTOR_X].value                = CurrentValues.TelescopeDirection.x;
    pMandatoryNumbers->np[ENTRY_VECTOR_Y].value                = CurrentValues.TelescopeDirection.y;
    pMandatoryNumbers->np[ENTRY_VECTOR_Z].value                = CurrentValues.TelescopeDirection.z;
    SetDriverBusy();
    BaseClient->sendNewNumber(pMandatoryNumbers);
    WaitForDriverCompletion();
    if (IPS_OK != pMandatoryNumbers->s)
    {
        IDLog("SendEntryData - Bad mandatory numbers state %s\n", pstateStr(pMandatoryNumbers->s));
        return false;
    }

    if ((0 != CurrentValues.PrivateDataSize) && (nullptr != CurrentValues.PrivateData.get()))
    {
        // I have a BLOB to send
        SetDriverBusy();
        BaseClient->startBlob(Device->getDeviceName(), pBLOB->name, timestamp());
        BaseClient->sendOneBlob(pBLOB->bp->name, CurrentValues.PrivateDataSize, pBLOB->bp->format,
                                CurrentValues.PrivateData.get());
        BaseClient->finishBlob();
        WaitForDriverCompletion();
        if (IPS_OK != pBLOB->s)
        {
            IDLog("SendEntryData - Bad BLOB state %s\n", pstateStr(pBLOB->s));
            return false;
        }
    }
    return true;
}

bool ClientAPIForAlignmentDatabase::SetDriverBusy()
{
    int ReturnCode = pthread_mutex_lock(&DriverActionCompleteMutex);

    if (ReturnCode != 0)
        return false;
    DriverActionComplete = false;
    IDLog("SetDriverBusy\n");
    ReturnCode = pthread_mutex_unlock(&DriverActionCompleteMutex);
    return ReturnCode == 0;
}

bool ClientAPIForAlignmentDatabase::SignalDriverCompletion()
{
    int ReturnCode = pthread_mutex_lock(&DriverActionCompleteMutex);

    if (ReturnCode != 0)
        return false;
    DriverActionComplete = true;
    ReturnCode           = pthread_cond_signal(&DriverActionCompleteCondition);
    if (ReturnCode != 0)
    {
        ReturnCode = pthread_mutex_unlock(&DriverActionCompleteMutex);
        return false;
    }
    IDLog("SignalDriverCompletion\n");
    ReturnCode = pthread_mutex_unlock(&DriverActionCompleteMutex);
    return ReturnCode == 0;
}

bool ClientAPIForAlignmentDatabase::WaitForDriverCompletion()
{
    int ReturnCode = pthread_mutex_lock(&DriverActionCompleteMutex);

    while (!DriverActionComplete)
    {
        IDLog("WaitForDriverCompletion - Waiting\n");
        ReturnCode = pthread_cond_wait(&DriverActionCompleteCondition, &DriverActionCompleteMutex);
        IDLog("WaitForDriverCompletion - Back from wait ReturnCode = %d\n", ReturnCode);
        if (ReturnCode != 0)
        {
            ReturnCode = pthread_mutex_unlock(&DriverActionCompleteMutex);
            return false;
        }
    }
    IDLog("WaitForDriverCompletion - Finished waiting\n");
    ReturnCode = pthread_mutex_unlock(&DriverActionCompleteMutex);
    return ReturnCode == 0;
}

} // namespace AlignmentSubsystem
} // namespace INDI
