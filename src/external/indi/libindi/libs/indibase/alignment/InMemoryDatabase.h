/*!
 * \file InMemoryDatabase.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#pragma once

#include "Common.h"

#include <libnova/ln_types.h>

#include <vector>

namespace INDI
{
namespace AlignmentSubsystem
{
/// \class InMemoryDatabase
/// \brief This class provides the driver side API to the in memory alignment database.
class InMemoryDatabase
{
  public:
    /// \brief Default constructor
    InMemoryDatabase();

    /// \brief Virtual destructor
    virtual ~InMemoryDatabase() {}

    typedef std::vector<AlignmentDatabaseEntry> AlignmentDatabaseType;

    // Public methods

    /// \brief Check if a entry already exists in the database
    /// \param[in] CandidateEntry The candidate entry to check
    /// \param[in] Tolerance The % tolerance used in the checking process (default 0.1%)
    /// \return True if an entry already exists within the required tolerance
    bool CheckForDuplicateSyncPoint(const AlignmentDatabaseEntry &CandidateEntry, double Tolerance = 0.1) const;

    /// \brief Get a reference to the in memory database.
    /// \return A reference to the in memory database.
    AlignmentDatabaseType &GetAlignmentDatabase() { return MySyncPoints; }

    /// \brief Get the database reference position
    /// \param[in] Position A pointer to a ln_lnlat_posn object to retunr the current position in
    /// \return True if successful
    bool GetDatabaseReferencePosition(ln_lnlat_posn &Position);

    /// \brief Load the database from persistent storage
    /// \param[in] DeviceName The name of the current device.
    /// \return True if successful
    bool LoadDatabase(const char *DeviceName);

    /// \brief Save the database to persistent storage
    /// \param[in] DeviceName The name of the current device.
    /// \return True if successful
    bool SaveDatabase(const char *DeviceName);

    /// \brief Set the database reference position
    /// \param[in] Latitude
    /// \param[in] Longitude
    void SetDatabaseReferencePosition(double Latitude, double Longitude);

    typedef void (*LoadDatabaseCallbackPointer_t)(void *);

    /// \brief Set the function to be called when the database is loaded or reloaded
    /// \param[in] CallbackPointer A pointer to the class function to call
    /// \param[in] ThisPointer A pointer to the class object of the callback function
    void SetLoadDatabaseCallback(LoadDatabaseCallbackPointer_t CallbackPointer, void *ThisPointer);

  private:
    AlignmentDatabaseType MySyncPoints;
    ln_lnlat_posn DatabaseReferencePosition;
    bool DatabaseReferencePositionIsValid;
    LoadDatabaseCallbackPointer_t LoadDatabaseCallback;
    void *LoadDatabaseCallbackThisPointer;
};

} // namespace AlignmentSubsystem
} // namespace INDI
