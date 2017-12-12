/*!
 * \file MathPluginManagement.h
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#pragma once

#include "BuiltInMathPlugin.h"

#include "inditelescope.h"

#include <memory>

namespace INDI
{
namespace AlignmentSubsystem
{
/*!
 * \class MathPluginManagement
 * \brief The following INDI properties are used to manage math plugins
 *  - ALIGNMENT_SUBSYSTEM_MATH_PLUGINS\n
 *  A list of available plugins (switch). This also allows the client to select a plugin.
 *  - ALIGNMENT_SUBSYSTEM_MATH_PLUGIN_INITIALISE\n
 *  Initialise or re-initialise the current math plugin.
 *  - ALIGNMENT_SUBSYSTEM_CURRENT_MATH_PLUGIN\n
 *  This is not communicated to the client and only used to save the currently selected plugin name
 *  to persistent storage.
 *
 *  This class also provides function links to the currently selected math plugin
 */
class MathPluginManagement : private MathPlugin // Derive from MathPluign to force the function signatures to match
{
  public:
    /** \enum MountType
            \brief Describes the basic type of the mount.
        */
    typedef enum MountType { EQUATORIAL, ALTAZ } MountType_t;

    /// \brief Default constructor
    MathPluginManagement();

    /// \brief Virtual destructor
    virtual ~MathPluginManagement() {}

    /** \brief Initialize alignment math plugin properties. It is recommended to call this function within initProperties() of your primary device
         * \param[in] pTelescope Pointer to the child INDI::Telecope class
        */
    void InitProperties(Telescope *pTelescope);

    /** \brief Call this function from within the ISNewSwitch processing path. The function will
         * handle any math plugin switch properties.
         * \param[in] pTelescope Pointer to the child INDI::Telecope class
         * \param[in] name vector property name
         * \param[in] states states as passed by the client
         * \param[in] names names as passed by the client
         * \param[in] n number of values and names pair to process.
        */
    void ProcessSwitchProperties(Telescope *pTelescope, const char *name, ISState *states, char *names[], int n);

    /** \brief Call this function from within the ISNewText processing path. The function will
         * handle any math plugin text properties. This text property is at the moment only contained in the
         * config file so this will normally only have work to do when the config file is loaded.
         * \param[in] pTelescope Pointer to the child INDI::Telecope class
         * \param[in] name vector property name
         * \param[in] texts texts as passed by the client
         * \param[in] names names as passed by the client
         * \param[in] n number of values and names pair to process.
        */
    void ProcessTextProperties(Telescope *pTelescope, const char *name, char *texts[], char *names[], int n);

    /** \brief Call this function to save persistent math plugin properties.
         * This function should be called from within the saveConfigItems function of your driver.
         * \param[in] fp File pointer passed into saveConfigItems
        */
    void SaveConfigProperties(FILE *fp);

    /** \brief Call this function to set the ApproximateMountAlignment property of the current
            Math Plugin. The alignment database should be initialised before this function is called
            so that it can use the DatabaseReferencePosition to determine which hemisphere the
            current observing site is in. For equatorial the ApproximateMountAlignment property
            will set to NORTH_CELESTIAL_POLE for sites in the northern hemisphere and SOUTH_CELESTIAL_POLE
            for sites in the southern hemisphere. For altaz mounts the ApproximateMountAlignment will
            be set to ZENITH.
            \param[in] Type the mount type either EQUATORIAL or ALTAZ
        */
    void SetApproximateMountAlignmentFromMountType(MountType_t Type);

    /// \brief Set the current in memory database
    /// \param[in] pDatabase A pointer to the current in memory database
    void SetCurrentInMemoryDatabase(InMemoryDatabase *pDatabase) { CurrentInMemoryDatabase = pDatabase; }

    /**
         * @brief SetAlignmentSubsystemActive Enable or Disable alignment subsystem
         * @param enable True to activate Alignment Subsystem. False to deactivate Alignment subsystem.
         */
    void SetAlignmentSubsystemActive(bool enable);

    /// \brief Return status of alignment subsystem
    /// \return True if active
    bool IsAlignmentSubsystemActive() const { return AlignmentSubsystemActive.s == ISS_ON ? true : false; }

    // These must match the function signatures in MathPlugin
    MountAlignment_t GetApproximateMountAlignment();
    bool Initialise(InMemoryDatabase *pInMemoryDatabase);
    void SetApproximateMountAlignment(MountAlignment_t ApproximateAlignment);
    bool TransformCelestialToTelescope(const double RightAscension, const double Declination, double JulianOffset,
                                       TelescopeDirectionVector &ApparentTelescopeDirectionVector);
    bool TransformTelescopeToCelestial(const TelescopeDirectionVector &ApparentTelescopeDirectionVector,
                                       double &RightAscension, double &Declination);

  private:
    void EnumeratePlugins();
    std::vector<std::string> MathPluginFiles;
    std::vector<std::string> MathPluginDisplayNames;

    std::unique_ptr<ISwitch> AlignmentSubsystemMathPlugins;
    ISwitchVectorProperty AlignmentSubsystemMathPluginsV;
    ISwitch AlignmentSubsystemMathPluginInitialise;
    ISwitchVectorProperty AlignmentSubsystemMathPluginInitialiseV;
    ISwitch AlignmentSubsystemActive;
    ISwitchVectorProperty AlignmentSubsystemActiveV;

    InMemoryDatabase *CurrentInMemoryDatabase;

    // The following property is used for configuration purposes only and is not propagated to the client
    IText AlignmentSubsystemCurrentMathPlugin;
    ITextVectorProperty AlignmentSubsystemCurrentMathPluginV;

    // The following hold links to the current loaded math plugin
    // These must match the function signatures in MathPlugin
    MountAlignment_t (MathPlugin::*pGetApproximateMountAlignment)();
    bool (MathPlugin::*pInitialise)(InMemoryDatabase *pInMemoryDatabase);
    void (MathPlugin::*pSetApproximateMountAlignment)(MountAlignment_t ApproximateAlignment);
    bool (MathPlugin::*pTransformCelestialToTelescope)(const double RightAscension, const double Declination,
                                                       double JulianOffset,
                                                       TelescopeDirectionVector &TelescopeDirectionVector);
    bool (MathPlugin::*pTransformTelescopeToCelestial)(const TelescopeDirectionVector &TelescopeDirectionVector,
                                                       double &RightAscension, double &Declination);
    MathPlugin *pLoadedMathPlugin;
    void *LoadedMathPluginHandle;

    BuiltInMathPlugin BuiltInPlugin;
};

} // namespace AlignmentSubsystem
} // namespace INDI
