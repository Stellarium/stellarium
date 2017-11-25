#ifndef TELESCOPECLIENTINDI_HPP
#define TELESCOPECLIENTINDI_HPP

#include "TelescopeClient.hpp"

#include "INDIConnection.hpp"

class TelescopeClientINDI final : public TelescopeClient
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

private:
    INDIConnection connection;
};

#endif // TELESCOPECLIENTINDI_HPP
