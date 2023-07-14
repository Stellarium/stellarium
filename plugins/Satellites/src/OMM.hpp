/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef SATELLITES_OMM_HPP
#define SATELLITES_OMM_HPP

#include <QMap>
#include <QChar>
#include <QString>
#include <QDateTime>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>
#include <QSharedPointer>
#include <QXmlStreamReader>

#include "OMMDateTime.hpp"

//! @class OMM
//! Auxiliary class for the %Satellites plugin.
//! @author Andy Kirkham
//! @ingroup satellites
//! 
//! The OMM (Orbit Mean-Elements Message) class provides an interface
//! to Earth orbiting satellite orbital elements. For many years these
//! have been transferred using a format known as TLE (two-line element).
//! However, newer formats are available, notably the standard OMM/XML
//! standard as well as JSON, CSV and others.
//! Since the legacy TLE format is restricted in Norad catalogue ID numbers
//! OMM/XML will become teh defacto standard at some point.
//! 
//! This class holds a single satellite elements and can decode from:-
//!     1. TLE
//!     2. XML OMM
//!     3. JSON OMM
class OMM
{
public:
	//! @enum OptStatus operational statuses
	enum class OptStatus
	{
		StatusUnknown = 0,
		StatusOperational          = 1,
		StatusNonoperational       = 2,
		StatusPartiallyOperational = 3,
		StatusStandby              = 4,
		StatusSpare                = 5,
		StatusExtendedMission      = 6,
		StatusDecayed              = 7
	};

	// Celestrak's "status code" list
	const QMap<QString, OptStatus> satOpStatusMap = 
	{ 
		{ "+", OptStatus::StatusOperational },
		{ "-", OptStatus::StatusNonoperational },
		{ "P", OptStatus::StatusPartiallyOperational },
		{ "B", OptStatus::StatusStandby },
		{ "S", OptStatus::StatusSpare },
		{ "X", OptStatus::StatusExtendedMission },
		{ "D", OptStatus::StatusDecayed },
		{ "?", OptStatus::StatusUnknown } 
	};

	enum class SourceType 
	{
		Invalid,
		LegacyTle,
		Xml,
		Json
	};

	OMM();
	OMM(QJsonObject&);
	OMM(const QVariantMap&);
	OMM(QXmlStreamReader& r);
	OMM(QString&, QString&);
	OMM(QString&, QString&, QString&);

	OMM(const OMM&);
	OMM& operator=(const OMM&);

	// Destructor.
	virtual ~OMM();

	virtual OMM& setLine0(const QString& s) { m_line0 = s; return *this; }
	virtual OMM& setLine1(const QString& s) { m_line1 = s; return *this; }
	virtual OMM& setLine2(const QString& s) { m_line2 = s; return *this; }

	virtual const QString& getLine0() const { return m_line0; }
	virtual const QString& getLine1() const { return m_line1; }
	virtual const QString& getLine2() const { return m_line2; }

	bool hasValidEpoch() { return m_epoch.getJulian() > 0.0; }

	bool hasValidLegacyTleData();

	void toJsonObj(QJsonObject&);

	void toVariantMap(QVariantMap& outmap);

	bool setFromXML(QXmlStreamReader& r);
	bool setFromJsonObj(const QJsonObject& obj);

	virtual SourceType getSourceType() { return m_source_type; }

	virtual double getEpochJD();
	virtual OMMDateTime getEpoch() { return m_epoch; }
	virtual double getMeanMotion() { return m_mean_motion; }
	virtual double getEccentricity() { return m_eccentricity; }
	virtual double getInclination() { return m_inclination; }
	virtual double getAscendingNode() { return m_ascending_node; }
	virtual double getArgumentOfPerigee() { return m_argument_perigee; }
	virtual double getMeanAnomoly() { return m_mean_anomoly; }
	virtual int getEphermisType() { return m_ephermeris_type; }
	virtual int getElementNumber() { return m_element_number; }

	virtual QChar getClassification() const { return m_classification; }
	virtual int getNoradcatId() { return m_norad_cat_id; }
	virtual int getRevAtEpoch() { return m_rev_at_epoch; }
	virtual double getBstar() { return m_bstar; }
	virtual double getMeanMotionDot() { return m_mean_motion_dot; }
	virtual double getMeanMotionDDot() { return m_mean_motion_ddot; }

	virtual const QString& getObjectName() const { return m_object_name; }
	virtual const QString& getObjectId() const { return m_object_id; }

	virtual OptStatus getStatus() { return m_status; }

	// Setter functions
	bool setEpoch(const QJsonValue & val, const QString & tag = "");
	bool setObjectName(const QJsonValue & val, const QString & tag = "");
	bool setObjectId(const QJsonValue& val, const QString& tag = "");
	bool setMeanMotion(const QJsonValue & val, const QString & tag = "");
	bool setEccentricity(const QJsonValue & val, const QString & tag = "");
	bool setInclination(const QJsonValue & val, const QString & tag = "");
	bool setAscendingNode(const QJsonValue & val, const QString & tag = "");
	bool setArgumentOfPerigee(const QJsonValue & val, const QString & tag = "");
	bool setMeanAnomoly(const QJsonValue & val, const QString & tag = "");
	bool setClassification(const QJsonValue & val, const QString & tag = "");
	bool setNoradcatId(const QJsonValue & val, const QString & tag = "");
	bool setRevAtEpoch(const QJsonValue & val, const QString & tag = "");
	bool setElementNumber(const QJsonValue & val, const QString & tag = "");
	bool setBstar(const QJsonValue & val, const QString & tag = "");
	bool setMeanMotionDot(const QJsonValue & val, const QString & tag = "");
	bool setMeanMotionDDot(const QJsonValue & val, const QString & tag = "");

	OMM &setStatus(OptStatus s) { m_status = s; return *this; } 

	virtual void processTleLegacy() {
		processTleLegacyLine0();
		processTleLegacyLine1();
		processTleLegacyLine2();
	}

private:
	void processTleLegacyLine0(void);
	void processTleLegacyLine1(void);
	void processTleLegacyLine2(void);
	void processTagElement(const QString& tag, const QJsonValue& val);

	SourceType m_source_type;
	
	// Legacy TLE data.
	QString m_line0{};
	QString m_line1{};
	QString m_line2{};

	// Mean elements.
	OMMDateTime m_epoch{};
	double m_mean_motion{};
	double m_eccentricity{};
	double m_inclination{};
	double m_ascending_node{};
	double m_argument_perigee{};
	double m_mean_anomoly{};

	// TLE parameters.
	QChar m_classification{};
	int m_norad_cat_id{};
	int m_rev_at_epoch{};
	double m_bstar{};
	double m_mean_motion_dot{};
	double m_mean_motion_ddot{};
	int m_ephermeris_type{};
	int m_element_number{};

	// Metadata
	QString m_object_name{};
	QString m_object_id{};

	// Celestrak status
	OptStatus m_status{};
};

#endif
