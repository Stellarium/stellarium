#ifndef INDICONNECTION_HPP
#define INDICONNECTION_HPP

#include "baseclient.h"

#include <mutex>

#include "VecMath.hpp"

class INDIConnection : public INDI::BaseClient
{
public:
    INDIConnection();

    Vec3d positionJNow() const;
    void setPositionJNow(Vec3d position);

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
    INDI::BaseDevice* mTelescope = nullptr;
    Vec3d mPosition = {0.0, 0.0, 0.0};
};

#endif // INDICONNECTION_HPP
