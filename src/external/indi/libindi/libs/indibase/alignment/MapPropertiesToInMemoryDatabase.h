/*!
 * \file MapPropertiesToInMemoryDatabase.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#pragma once

#include "InMemoryDatabase.h"

#include "inditelescope.h"

namespace INDI
{
namespace AlignmentSubsystem
{
/*!
 * \class MapPropertiesToInMemoryDatabase
 * \brief An entry in the sync point database is defined by the following INDI properties
 * - ALIGNMENT_POINT_ENTRY_OBSERVATION_JULIAN_DATE\n
 *   The Julian date of the sync point observation (number)
 * - ALIGNMENT_POINT_ENTRY_OBSERVATION_LOCAL_SIDEREAL_TIME\n
 *   The local sidereal time of the sync point observation (number)
 * - ALIGNMENT_POINT_ENTRY_RA\n
 *   The right ascension of the sync point (number)
 * - ALIGNMENT_POINT_ENTRY_DEC\n
 *   The declination of the sync point (number)
 * - ALIGNMENT_POINT_ENTRY_VECTOR_X\n
 *   The x component of the telescope direction vector of the sync point (number)
 * - ALIGNMENT_POINT_ENTRY_VECTOR_Y\n
 *   The y component of the telescope direction vector of the sync point (number)
 * - ALIGNMENT_POINT_ENTRY_VECTOR_Z\n
 *   The z component of the telescope direction vector of the sync point (number)
 * - ALIGNMENT_POINT_ENTRY_PRIVATE\n
 *   An optional binary blob for communication between the client and the math plugin
 * .
 * The database is accessed using the following properties
 * - ALIGNMENT_POINTSET_SIZE\n
 *   The count of the number of sync points in the set (number)
 * - ALIGNMENT_POINTSET_CURRENT_ENTRY\n
 *   A zero based number that sets/shows the current entry (number)
 *   Only valid if ALIGNMENT_POINTSET_SIZE is greater than zero
 * - ALIGNMENT_POINTSET_ACTION\n
 *   Determines the action to take when the COMMIT property is written
 *   - APPEND\n
 *     Append a new entry to the set.
 *   - INSERT\n
 *     Insert a new entry at the pointer.
 *   - EDIT\n
 *     Overwrites the entry at the pointer.
 *   - DELETE\n
 *     Delete the entry at the pointer.
 *   - CLEAR\n
 *     Delete all entries.
 *   - READ\n
 *     Read the entry at the pointer.
 *   - READ INCREMENT\n
 *     Increment the pointer before reading the entry.
 *   - LOAD DATABASE\n
 *     Load the databse from local storage.
 *   - SAVE DATABASE\n
 *     Save the database to local storage.
 * - ALIGNMENT_POINTSET_COMMIT\n
 *   When written take the action defined above.
 *   - COMMIT
 *
 */
class MapPropertiesToInMemoryDatabase : public InMemoryDatabase
{
  public:
    /// \brief Virtual destructor
    virtual ~MapPropertiesToInMemoryDatabase() {}

    // Public methods

    /// \brief Initialize alignment database properties. It is recommended to call this function within initProperties()
    /// of your primary device
    /// \param[in] pTelescope Pointer to the child INDI::Telecope class
    void InitProperties(Telescope *pTelescope);

    /// \brief Call this function from within the ISNewBLOB processing path. The function will
    /// handle any alignment database related properties.
    /// \param[in] pTelescope Pointer to the child INDI::Telecope class
    /// \param[in] name vector property name
    /// \param[in] sizes
    /// \param[in] blobsizes
    /// \param[in] blobs
    /// \param[in] formats
    /// \param[in] names
    /// \param[in] n
    void ProcessBlobProperties(Telescope *pTelescope, const char *name, int sizes[], int blobsizes[], char *blobs[],
                               char *formats[], char *names[], int n);

    /// \brief Call this function from within the ISNewNumber processing path. The function will
    /// handle any alignment database related properties.
    /// \param[in] pTelescope Pointer to the child INDI::Telecope class
    /// \param[in] name vector property name
    /// \param[in] values value as passed by the client
    /// \param[in] names names as passed by the client
    /// \param[in] n number of values and names pair to process.
    void ProcessNumberProperties(Telescope *pTelescope, const char *name, double values[], char *names[], int n);

    /// \brief Call this function from within the ISNewSwitch processing path. The function will
    /// handle any alignment database related properties.
    /// \param[in] pTelescope Pointer to the child INDI::Telecope class
    /// \param[in] name vector property name
    /// \param[in] states states as passed by the client
    /// \param[in] names names as passed by the client
    /// \param[in] n number of values and names pair to process.
    void ProcessSwitchProperties(Telescope *pTelescope, const char *name, ISState *states, char *names[], int n);

    /// \brief Call this function from within the updateLocation processing path
    /// \param[in] latitude Site latitude in degrees.
    /// \param[in] longitude Site latitude in degrees increasing eastward from Greenwich (0 to 360).
    /// \param[in] elevation Site elevation in meters.
    void UpdateLocation(double latitude, double longitude, double elevation);

    /// \brief Call this function when the number of entries in the database changes
    void UpdateSize();

  private:
    INumber AlignmentPointSetEntry[6];
    INumberVectorProperty AlignmentPointSetEntryV;
    IBLOB AlignmentPointSetPrivateBinaryData;
    IBLOBVectorProperty AlignmentPointSetPrivateBinaryDataV;
    INumber AlignmentPointSetSize;
    INumberVectorProperty AlignmentPointSetSizeV;
    INumber AlignmentPointSetPointer;
    INumberVectorProperty AlignmentPointSetPointerV;
    ISwitch AlignmentPointSetAction[9];
    ISwitchVectorProperty AlignmentPointSetActionV;
    ISwitch AlignmentPointSetCommit;
    ISwitchVectorProperty AlignmentPointSetCommitV;
};

} // namespace AlignmentSubsystem
} // namespace INDI
