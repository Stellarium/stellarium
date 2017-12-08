#include "MathPluginManagerClient.h"

#include <iostream>

#include <cstring>
#include <sstream>

using namespace INDI::AlignmentSubsystem;

MathPluginManagerClient::MathPluginManagerClient() : DeviceName("skywatcherAPIMount")
{
    //ctor
}

MathPluginManagerClient::~MathPluginManagerClient()
{
    //dtor
}

// Public methods

void MathPluginManagerClient::Initialise(int argc, char *argv[])
{
    std::string HostName("localhost");
    int Port = 7624;

    if (argc > 1)
        DeviceName = argv[1];
    if (argc > 2)
        HostName = argv[2];
    if (argc > 3)
    {
        std::istringstream Parameter(argv[3]);
        Parameter >> Port;
    }

    AlignmentSubsystemForClients::Initialise(DeviceName.c_str(), this);

    setServer(HostName.c_str(), Port);

    watchDevice(DeviceName.c_str());

    connectServer();

    setBLOBMode(B_ALSO, DeviceName.c_str(), nullptr);
}

void MathPluginManagerClient::Test()
{
    MathPluginsList AvailableMathPlugins;

    cout << "Testing Enumerate available plugins\n";

    if (EnumerateMathPlugins(AvailableMathPlugins))
    {
        cout << "Success - List of plugins follows\n";
        for (MathPluginsList::const_iterator iTr = AvailableMathPlugins.begin(); iTr != AvailableMathPlugins.end();
             iTr++)
            cout << *iTr << '\n';
    }
    else
        cout << "Failure\n";

    cout << "Testing select plugin\n";

    if (SelectMathPlugin(AvailableMathPlugins[AvailableMathPlugins.size() - 1]))
        cout << "Success\n";
    else
        cout << "Failure\n";

    cout << "Testing reinitilialise plugin\n";

    if (ReInitialiseMathPlugin())
        cout << "Success\n";
    else
        cout << "Failure\n";
}

// Protected methods

void MathPluginManagerClient::newDevice(INDI::BaseDevice *dp)
{
    ProcessNewDevice(dp);
}

void MathPluginManagerClient::newProperty(INDI::Property *property)
{
    ProcessNewProperty(property);
}

void MathPluginManagerClient::newSwitch(ISwitchVectorProperty *svp)
{
    ProcessNewSwitch(svp);
}
