/*
 * Stellarium Atlante
 * Module Cosmologique Atlante
 */

#ifndef ATLANTECOSMOLOGY_HPP
#define ATLANTECOSMOLOGY_HPP

#include <QObject>
#include <QString>
#include "VecMath.hpp"

class Planet;

enum class CosmologyMode
{
	Standard = 0,
	AtlanteGeocentric = 1
};

class AtlanteCosmology : public QObject
{
	Q_OBJECT
	Q_PROPERTY(CosmologyMode mode READ getMode WRITE setMode NOTIFY modeChanged)
	Q_PROPERTY(bool isAtlanteGeocentric READ isAtlanteGeocentric NOTIFY modeChanged)

public:
	static AtlanteCosmology* getInstance();

	explicit AtlanteCosmology(QObject* parent = nullptr);
	~AtlanteCosmology() override;

	CosmologyMode getMode() const { return currentMode; }
	bool isAtlanteGeocentric() const { return currentMode == CosmologyMode::AtlanteGeocentric; }

	// Transformation géocentrique Atlante :
	// - Terre : (0,0,0)
	// - Soleil : -r_helio(Terre)
	// - Autre corps P : r_helio(P) - r_helio(Terre)
	static Vec3d computeGeocentricPos(const Vec3d& helioPos, const Vec3d& earthHelioPos);
	static Vec3d getAtlantePos(const Planet* body);

public slots:
	void setMode(CosmologyMode mode);
	void setModeFromInt(int modeInt);
	void setAtlanteGeocentric(bool enabled);

signals:
	void modeChanged(CosmologyMode mode);

private:
	CosmologyMode currentMode;
	static AtlanteCosmology* instance;
};

#endif // ATLANTECOSMOLOGY_HPP
