#ifndef TELESCOPECLIENTINDI_HPP
#define TELESCOPECLIENTINDI_HPP

#include "TelescopeClient.hpp"
#include "baseclient.h"

class TelescopeClientINDI final : public TelescopeClient, INDI::BaseClient
{
public:
    TelescopeClientINDI(const QString &name);
    ~TelescopeClientINDI();

    // StelObject interface
public:
    Vec3d getJ2000EquatorialPos(const StelCore *core) const override;

    // TelescopeClient interface
public:
    void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override;
    bool isConnected() const override;
    bool hasKnownPosition() const override;

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
};

#endif // TELESCOPECLIENTINDI_HPP
