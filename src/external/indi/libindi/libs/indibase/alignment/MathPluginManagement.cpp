/*!
 * \file MathPluginManagement.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "MathPluginManagement.h"

#include <dirent.h>
#include <dlfcn.h>
#include <cerrno>

namespace INDI
{
namespace AlignmentSubsystem
{
MathPluginManagement::MathPluginManagement() : CurrentInMemoryDatabase(nullptr),
    pGetApproximateMountAlignment(&MathPlugin::GetApproximateMountAlignment),
    pInitialise(&MathPlugin::Initialise),
    pSetApproximateMountAlignment(&MathPlugin::SetApproximateMountAlignment),
    pTransformCelestialToTelescope(&MathPlugin::TransformCelestialToTelescope),
    pTransformTelescopeToCelestial(&MathPlugin::TransformTelescopeToCelestial),
    pLoadedMathPlugin(&BuiltInPlugin), LoadedMathPluginHandle(nullptr)
{
}

void MathPluginManagement::InitProperties(Telescope *ChildTelescope)
{
    EnumeratePlugins();
    AlignmentSubsystemMathPlugins.reset(new ISwitch[MathPluginDisplayNames.size() + 1]);
    IUFillSwitch(AlignmentSubsystemMathPlugins.get(), "INBUILT_MATH_PLUGIN", "Inbuilt Math Plugin", ISS_ON);

    for (int i = 0; i < (int)MathPluginDisplayNames.size(); i++)
    {
        IUFillSwitch(AlignmentSubsystemMathPlugins.get() + i + 1, MathPluginDisplayNames[i].c_str(),
                     MathPluginDisplayNames[i].c_str(), ISS_OFF);
    }
    IUFillSwitchVector(&AlignmentSubsystemMathPluginsV, AlignmentSubsystemMathPlugins.get(),
                       MathPluginDisplayNames.size() + 1, ChildTelescope->getDeviceName(),
                       "ALIGNMENT_SUBSYSTEM_MATH_PLUGINS", "Math Plugins", ALIGNMENT_TAB, IP_RW, ISR_1OFMANY, 60,
                       IPS_IDLE);
    ChildTelescope->registerProperty(&AlignmentSubsystemMathPluginsV, INDI_SWITCH);

    IUFillSwitch(&AlignmentSubsystemMathPluginInitialise, "ALIGNMENT_SUBSYSTEM_MATH_PLUGIN_INITIALISE", "OK", ISS_OFF);
    IUFillSwitchVector(&AlignmentSubsystemMathPluginInitialiseV, &AlignmentSubsystemMathPluginInitialise, 1,
                       ChildTelescope->getDeviceName(), "ALIGNMENT_SUBSYSTEM_MATH_PLUGIN_INITIALISE",
                       "(Re)Initialise Plugin", ALIGNMENT_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);
    ChildTelescope->registerProperty(&AlignmentSubsystemMathPluginInitialiseV, INDI_SWITCH);

    IUFillSwitch(&AlignmentSubsystemActive, "ALIGNMENT SUBSYSTEM ACTIVE", "Alignment Subsystem Active", ISS_OFF);
    IUFillSwitchVector(&AlignmentSubsystemActiveV, &AlignmentSubsystemActive, 1, ChildTelescope->getDeviceName(),
                       "ALIGNMENT_SUBSYSTEM_ACTIVE", "Activate alignment subsystem", ALIGNMENT_TAB, IP_RW, ISR_ATMOST1,
                       60, IPS_IDLE);
    ChildTelescope->registerProperty(&AlignmentSubsystemActiveV, INDI_SWITCH);

    // The following property is used for configuration purposes only and is not exposed to the client.
    IUFillText(&AlignmentSubsystemCurrentMathPlugin, "ALIGNMENT_SUBSYSTEM_CURRENT_MATH_PLUGIN", "Current Math Plugin",
               AlignmentSubsystemMathPlugins.get()[0].label);
    IUFillTextVector(&AlignmentSubsystemCurrentMathPluginV, &AlignmentSubsystemCurrentMathPlugin, 1,
                     ChildTelescope->getDeviceName(), "ALIGNMENT_SUBSYSTEM_CURRENT_MATH_PLUGIN", "Current Math Plugin",
                     ALIGNMENT_TAB, IP_RO, 60, IPS_IDLE);
}

void MathPluginManagement::ProcessTextProperties(Telescope *pTelescope, const char *name, char *texts[], char *names[],
                                                 int n)
{
    DEBUGFDEVICE(pTelescope->getDeviceName(), INDI::Logger::DBG_DEBUG, "ProcessTextProperties - name(%s)", name);
    if (strcmp(name, AlignmentSubsystemCurrentMathPluginV.name) == 0)
    {
        AlignmentSubsystemCurrentMathPluginV.s = IPS_OK;
        IUUpdateText(&AlignmentSubsystemCurrentMathPluginV, texts, names, n);

        if (0 != strcmp(AlignmentSubsystemMathPlugins.get()[0].label, AlignmentSubsystemCurrentMathPlugin.text))
        {
            // Unload old plugin if required
            if (nullptr != LoadedMathPluginHandle)
            {
                typedef void Destroy_t(MathPlugin *);
                Destroy_t *Destroy = (Destroy_t *)dlsym(LoadedMathPluginHandle, "Destroy");
                if (nullptr != Destroy)
                {
                    Destroy(pLoadedMathPlugin);
                    pLoadedMathPlugin = nullptr;
                    if (0 == dlclose(LoadedMathPluginHandle))
                    {
                        LoadedMathPluginHandle = nullptr;
                    }
                    else
                    {
                        IDLog("MathPluginManagement - dlclose failed on loaded plugin - %s\n", dlerror());
                        AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                    }
                }
                else
                {
                    IDLog("MathPluginManagement - cannot get Destroy function - %s\n", dlerror());
                    AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                }
            }
            // It is not the built in so try to load it
            if (nullptr != (LoadedMathPluginHandle = dlopen(AlignmentSubsystemCurrentMathPlugin.text, RTLD_NOW)))
            {
                typedef MathPlugin *Create_t();
                Create_t *Create = (Create_t *)dlsym(LoadedMathPluginHandle, "Create");
                if (nullptr != Create)
                {
                    pLoadedMathPlugin = Create();

                    // TODO - Update the client to reflect the new plugin
                    int i = 0;

                    for (i = 0; i < (int)MathPluginFiles.size(); i++)
                    {
                        if (0 == strcmp(AlignmentSubsystemCurrentMathPlugin.text, MathPluginFiles[i].c_str()))
                            break;
                    }
                    if (i < (int)MathPluginFiles.size())
                    {
                        IUResetSwitch(&AlignmentSubsystemMathPluginsV);
                        (AlignmentSubsystemMathPlugins.get() + i + 1)->s = ISS_ON;
                        //  Update client
                        IDSetSwitch(&AlignmentSubsystemMathPluginsV, nullptr);
                    }
                    else
                    {
                        IDLog("MathPluginManagement - cannot find %s in list of plugins\n", MathPluginFiles[i].c_str());
                    }
                }
                else
                {
                    IDLog("MathPluginManagement - cannot get Create function - %s\n", dlerror());
                }
            }
            else
            {
                IDLog("MathPluginManagement - cannot load plugin %s error %s\n",
                      AlignmentSubsystemCurrentMathPlugin.text, dlerror());
            }
        }
        else
        {
            // It is the inbuilt plugin
            // Unload old plugin if required
            if (nullptr != LoadedMathPluginHandle)
            {
                typedef void Destroy_t(MathPlugin *);
                Destroy_t *Destroy = (Destroy_t *)dlsym(LoadedMathPluginHandle, "Destroy");
                if (nullptr != Destroy)
                {
                    Destroy(pLoadedMathPlugin);
                    pLoadedMathPlugin = nullptr;
                    if (0 == dlclose(LoadedMathPluginHandle))
                    {
                        LoadedMathPluginHandle = nullptr;
                    }
                    else
                    {
                        IDLog("MathPluginManagement - dlclose failed on loaded plugin - %s\n", dlerror());
                        AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                    }
                }
                else
                {
                    IDLog("MathPluginManagement - cannot get Destroy function - %s\n", dlerror());
                    AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                }
            }
            pLoadedMathPlugin = &BuiltInPlugin;
            IUResetSwitch(&AlignmentSubsystemMathPluginsV);
            AlignmentSubsystemMathPlugins.get()->s = ISS_ON;
            //  Update client
            IDSetSwitch(&AlignmentSubsystemMathPluginsV, nullptr);
        }
    }
}

void MathPluginManagement::ProcessSwitchProperties(Telescope *pTelescope, const char *name, ISState *states,
                                                   char *names[], int n)
{
    //DEBUGFDEVICE(pTelescope->getDeviceName(), INDI::Logger::DBG_DEBUG, "ProcessSwitchProperties - name(%s)", name);
    INDI_UNUSED(pTelescope);
    if (strcmp(name, AlignmentSubsystemMathPluginsV.name) == 0)
    {
        int CurrentPlugin = IUFindOnSwitchIndex(&AlignmentSubsystemMathPluginsV);
        IUUpdateSwitch(&AlignmentSubsystemMathPluginsV, states, names, n);
        AlignmentSubsystemMathPluginsV.s = IPS_OK; // Assume OK for the time being
        int NewPlugin                    = IUFindOnSwitchIndex(&AlignmentSubsystemMathPluginsV);
        if (NewPlugin != CurrentPlugin)
        {
            // New plugin requested
            // Unload old plugin if required
            if (0 != CurrentPlugin)
            {
                typedef void Destroy_t(MathPlugin *);
                Destroy_t *Destroy = (Destroy_t *)dlsym(LoadedMathPluginHandle, "Destroy");
                if (nullptr != Destroy)
                {
                    Destroy(pLoadedMathPlugin);
                    pLoadedMathPlugin = nullptr;
                    if (0 == dlclose(LoadedMathPluginHandle))
                    {
                        LoadedMathPluginHandle = nullptr;
                    }
                    else
                    {
                        IDLog("MathPluginManagement - dlclose failed on loaded plugin - %s\n", dlerror());
                        AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                    }
                }
                else
                {
                    IDLog("MathPluginManagement - cannot get Destroy function - %s\n", dlerror());
                    AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                }
            }
            // Load the requested plugin if required
            if (0 != NewPlugin)
            {
                std::string PluginPath(MathPluginFiles[NewPlugin - 1]);
                if (nullptr != (LoadedMathPluginHandle = dlopen(PluginPath.c_str(), RTLD_NOW)))
                {
                    typedef MathPlugin *Create_t();
                    Create_t *Create = (Create_t *)dlsym(LoadedMathPluginHandle, "Create");
                    if (nullptr != Create)
                    {
                        pLoadedMathPlugin = Create();
                        IUSaveText(&AlignmentSubsystemCurrentMathPlugin, PluginPath.c_str());
                    }
                    else
                    {
                        IDLog("MathPluginManagement - cannot get Create function - %s\n", dlerror());
                        AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                    }
                }
                else
                {
                    IDLog("MathPluginManagement - cannot load plugin %s error %s\n", PluginPath.c_str(), dlerror());
                    AlignmentSubsystemMathPluginsV.s = IPS_ALERT;
                }
            }
            else
            {
                // It is in built plugin just set up the pointers
                pLoadedMathPlugin = &BuiltInPlugin;
            }
        }

        //  Update client
        IDSetSwitch(&AlignmentSubsystemMathPluginsV, nullptr);
    }
    else if (strcmp(name, AlignmentSubsystemMathPluginInitialiseV.name) == 0)
    {
        AlignmentSubsystemMathPluginInitialiseV.s = IPS_OK;
        IUResetSwitch(&AlignmentSubsystemMathPluginInitialiseV);
        //  Update client display
        IDSetSwitch(&AlignmentSubsystemMathPluginInitialiseV, nullptr);

        // Initialise or reinitialise the current math plugin
        Initialise(CurrentInMemoryDatabase);
    }
    else if (strcmp(name, AlignmentSubsystemActiveV.name) == 0)
    {
        AlignmentSubsystemActiveV.s = IPS_OK;
        if (0 == IUUpdateSwitch(&AlignmentSubsystemActiveV, states, names, n))
            //  Update client
            IDSetSwitch(&AlignmentSubsystemActiveV, nullptr);
    }
}

void MathPluginManagement::SetAlignmentSubsystemActive(bool enable)
{
    AlignmentSubsystemActive.s  = enable ? ISS_ON : ISS_OFF;
    AlignmentSubsystemActiveV.s = IPS_OK;
    IDSetSwitch(&AlignmentSubsystemActiveV, nullptr);
}

void MathPluginManagement::SaveConfigProperties(FILE *fp)
{
    IUSaveConfigText(fp, &AlignmentSubsystemCurrentMathPluginV);
    IUSaveConfigSwitch(fp, &AlignmentSubsystemActiveV);
}

void MathPluginManagement::SetApproximateMountAlignmentFromMountType(MountType_t Type)
{
    if (EQUATORIAL == Type)
    {
        ln_lnlat_posn Position { 0, 0 };
        if (CurrentInMemoryDatabase->GetDatabaseReferencePosition(Position))
        {
            if (Position.lat >= 0)
                SetApproximateMountAlignment(NORTH_CELESTIAL_POLE);
            else
                SetApproximateMountAlignment(SOUTH_CELESTIAL_POLE);
        }
        //else
        // TODO some kind of error!!!
    }
    else
        SetApproximateMountAlignment(ZENITH);
}

// These must match the function signatures in MathPlugin

MountAlignment_t MathPluginManagement::GetApproximateMountAlignment()
{
    return (pLoadedMathPlugin->*pGetApproximateMountAlignment)();
}

bool MathPluginManagement::Initialise(InMemoryDatabase *pInMemoryDatabase)
{
    return (pLoadedMathPlugin->*pInitialise)(pInMemoryDatabase);
}

void MathPluginManagement::SetApproximateMountAlignment(MountAlignment_t ApproximateAlignment)
{
    (pLoadedMathPlugin->*pSetApproximateMountAlignment)(ApproximateAlignment);
}

bool MathPluginManagement::TransformCelestialToTelescope(const double RightAscension, const double Declination,
                                                         double JulianOffset,
                                                         TelescopeDirectionVector &ApparentTelescopeDirectionVector)
{
    if (AlignmentSubsystemActive.s == ISS_ON)
        return (pLoadedMathPlugin->*pTransformCelestialToTelescope)(RightAscension, Declination, JulianOffset,
                                                                    ApparentTelescopeDirectionVector);
    else
        return false;
}

bool MathPluginManagement::TransformTelescopeToCelestial(
    const TelescopeDirectionVector &ApparentTelescopeDirectionVector, double &RightAscension, double &Declination)
{
    if (AlignmentSubsystemActive.s == ISS_ON)
        return (pLoadedMathPlugin->*pTransformTelescopeToCelestial)(ApparentTelescopeDirectionVector, RightAscension,
                                                                    Declination);
    else
        return false;
}

void MathPluginManagement::EnumeratePlugins()
{
    MathPluginFiles.clear();
    MathPluginDisplayNames.clear();
#ifndef OSX_EMBEDED_MODE
    dirent *de;
    DIR *dp;

    errno = 0;
    dp    = opendir(INDI_MATH_PLUGINS_DIRECTORY);
    if (dp)
    {
        while (true)
        {
            void *Handle;
            std::string PluginPath(INDI_MATH_PLUGINS_DIRECTORY "/");

            errno = 0;
            de    = readdir(dp);
            if (de == nullptr)
                break;
            if (0 == strcmp(de->d_name, "."))
                continue;
            if (0 == strcmp(de->d_name, ".."))
                continue;

            // Try to load the plugin
            PluginPath.append(de->d_name);
            Handle = dlopen(PluginPath.c_str(), RTLD_NOW);
            if (nullptr == Handle)
            {
                IDLog("EnumeratePlugins - cannot load plugin %s error %s\n", PluginPath.c_str(), dlerror());
                continue;
            }

            // Try to get the plugin display name
            typedef const char *GetDisplayName_t();
            GetDisplayName_t *GetDisplayNamePtr = (GetDisplayName_t *)dlsym(Handle, "GetDisplayName");
            if (nullptr == GetDisplayNamePtr)
            {
                IDLog("EnumeratePlugins - cannot get plugin %s DisplayName error %s\n", PluginPath.c_str(), dlerror());
                continue;
            }
            IDLog("EnumeratePlugins - found plugin %s\n", GetDisplayNamePtr());

            MathPluginDisplayNames.push_back(GetDisplayNamePtr());
            MathPluginFiles.push_back(PluginPath);
            dlclose(Handle);
        }
        closedir(dp);
    }
    else
    {
        IDLog("EnumeratePlugins - Failed to open %s error %s\n", INDI_MATH_PLUGINS_DIRECTORY, strerror(errno));
    }
#endif
}

} // namespace AlignmentSubsystem
} // namespace INDI
