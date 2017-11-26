#ifndef INDICONNECTION_HPP
#define INDICONNECTION_HPP

#include "baseclient.h"

#include <mutex>

#include "VecMath.hpp"

class INDIConnection : public INDI::BaseClient
{
public:
    typedef struct
    {
        double RA = 0.0;
        double DEC = 0.0;
    } Coordinates;

    INDIConnection();

    Coordinates position() const;
    void setPosition(Coordinates coords);

    bool isConnected() const;

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
    Coordinates mCoordinatesJNow;
};

#endif // INDICONNECTION_HPP
