#ifndef INDICONNECTION_HPP
#define INDICONNECTION_HPP

#include "baseclient.h"

#include <mutex>

class INDIConnection : public INDI::BaseClient
{
public:
    INDIConnection();

    double declination() const;
    double rightAscension() const;

    // INDI::BaseMediator interface
public:
    void newDevice(INDI::BaseDevice *dp) override;
    void removeDevice(INDI::BaseDevice *dp) override;
    void newProperty(INDI::Property *property) override;
    void removeProperty(INDI::Property *property) override;
    void newBLOB(IBLOB *bp) override;
    void newSwitch(ISwitchVectorProperty *svp) override;
    void newNumber(INumberVectorProperty *nvp) override;
    void newText(ITextVectorProperty *tvp) override;
    void newLight(ILightVectorProperty *lvp) override;
    void newMessage(INDI::BaseDevice *dp, int messageID) override;
    void serverConnected() override;
    void serverDisconnected(int exit_code) override;

private:
    mutable std::mutex mMutex;
    double mDeclination = 0.0;
    double mRightAscension = 0.0;
};

#endif // INDICONNECTION_HPP
