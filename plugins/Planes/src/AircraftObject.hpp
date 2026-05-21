#ifndef PLANES_AIRCRAFTOBJECT_HPP
#define PLANES_AIRCRAFTOBJECT_HPP

#include "AircraftRecord.hpp"
#include "StelObject.hpp"
#include "StelProjectorType.hpp"
#include "StelTranslator.hpp"

#include <QSharedPointer>
#include <QString>

class StelPainter;

class AircraftObject : public StelObject
{
public:
	static const QString STEL_TYPE;

	explicit AircraftObject(const AircraftRecord& record);
	void updateRecord(const AircraftRecord& record);

	QString getType() const override { return STEL_TYPE; }
	QString getObjectType() const override { return N_("plane"); }
	QString getObjectTypeI18n() const override;
	QString getID() const override;
	QString getEnglishName() const override;
	QString getNameI18n() const override;
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	Vec3f getInfoColor() const override;
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	float getVMagnitude(const StelCore* core) const override;
	double getAngularRadius(const StelCore* core) const override;
	float getSelectPriority(const StelCore* core) const override;

	bool isAboveHorizon(const StelCore* core) const;
	void draw(StelCore* core, StelPainter* painter, bool drawLabels, int labelMode) const;

	const AircraftRecord& record() const { return aircraftRecord; }

private:
	AircraftRecord getExtrapolatedRecord(double elapsedSeconds) const;
	double getElapsedSeconds() const;
	Vec3d getAltAzPos(const StelCore* core, double elapsedSeconds=0.0) const;
	float getScreenRotationDegrees(StelCore* core, const StelProjectorP& projector, const Vec3d& currentScreenPos) const;
	QString labelText() const;
	QString displayLabelText(int labelMode) const;

	AircraftRecord aircraftRecord;
};

using AircraftObjectP = QSharedPointer<AircraftObject>;

#endif
