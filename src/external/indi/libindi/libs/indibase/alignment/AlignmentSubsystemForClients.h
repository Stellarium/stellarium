/*!
 * \file AlignmentSubsystemForClients.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 * This file provides a shorthand way for clients to include all the
 * functionality they need to use the INDI Alignment Subsystem
 * Clients should inherit this class alongside INDI::BaseClient
 */

#pragma once

#include "ClientAPIForAlignmentDatabase.h"
#include "ClientAPIForMathPluginManagement.h"
#include "TelescopeDirectionVectorSupportFunctions.h"

#include "basedevice.h"

namespace INDI
{
namespace AlignmentSubsystem
{
/*!
 * \class AlignmentSubsystemForClients
 * \brief This class encapsulates all the alignment subsystem classes that are useful to client implementations.
 * Clients should inherit from this class.
 */
class AlignmentSubsystemForClients : public ClientAPIForMathPluginManagement,
                                     public ClientAPIForAlignmentDatabase,
                                     public TelescopeDirectionVectorSupportFunctions
{
  public:
    /// \brief Virtual destructor
    virtual ~AlignmentSubsystemForClients() {}

    /** \brief This routine should be called before any connections to devices are made.
            \param[in] DeviceName The device name of INDI driver instance to be used.
            \param[in] BaseClient A pointer to the child INDI::BaseClient class
        */
    void Initialise(const char *DeviceName, INDI::BaseClient *BaseClient);

    /** \brief Process new BLOB message from driver. This routine should be called from within
         the newBLOB handler in the client.
            \param[in] BLOBPointer A pointer to the INDI::IBLOB.
        */
    void ProcessNewBLOB(IBLOB *BLOBPointer);

    /** \brief Process new device message from driver. This routine should be called from within
         the newDevice handler in the client.
            \param[in] DevicePointer A pointer to the INDI::BaseDevice object.
        */
    void ProcessNewDevice(INDI::BaseDevice *DevicePointer);

    /** \brief Process new property message from driver. This routine should be called from within
         the newProperty handler in the client.
            \param[in] PropertyPointer A pointer to the INDI::Property object.
        */
    void ProcessNewProperty(INDI::Property *PropertyPointer);

    /** \brief Process new number message from driver. This routine should be called from within
         the newNumber handler in the client.
            \param[in] NumberVectorPropertyPointer A pointer to the INDI::INumberVectorProperty.
        */
    void ProcessNewNumber(INumberVectorProperty *NumberVectorPropertyPointer);

    /** \brief Process new switch message from driver. This routine should be called from within
         the newSwitch handler in the client.
            \param[in] SwitchVectorPropertyPointer A pointer to the INDI::ISwitchVectorProperty.
        */
    void ProcessNewSwitch(ISwitchVectorProperty *SwitchVectorPropertyPointer);

  private:
    std::string DeviceName;
};

} // namespace AlignmentSubsystem
} // namespace INDI
