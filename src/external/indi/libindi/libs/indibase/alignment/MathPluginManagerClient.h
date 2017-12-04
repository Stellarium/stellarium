
#pragma once

#include "indibase/baseclient.h"
#include "indibase/basedevice.h"

#include "indibase/alignment/AlignmentSubsystemForClients.h"

class MathPluginManagerClient : public INDI::BaseClient, INDI::AlignmentSubsystem::AlignmentSubsystemForClients
{
  public:
    MathPluginManagerClient();
    virtual ~MathPluginManagerClient();

    // Public methods

    void Initialise(int argc, char *argv[]);

    void Test();

  protected:
    // Protected methods

    virtual void newBLOB(IBLOB *bp) {}
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID) {}
    virtual void newNumber(INumberVectorProperty *nvp) {}
    virtual void newProperty(INDI::Property *property);
    virtual void newSwitch(ISwitchVectorProperty *svp);
    virtual void newText(ITextVectorProperty *tvp) {}
    virtual void newLight(ILightVectorProperty *lvp) {}
    virtual void removeProperty(INDI::Property *property) {}
    virtual void serverConnected() {}
    virtual void serverDisconnected(int exit_code) {}

  private:
    INDI::BaseDevice *Device;
    std::string DeviceName;
};
