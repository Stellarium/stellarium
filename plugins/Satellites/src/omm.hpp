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

#include <QChar>
#include <QString>
#include <QDateTime>
#include <QXmlStreamReader>

namespace PluginSatellites {

class omm
{
private:
	omm(); // Make default constructor unavailable.

public:

	enum class SourceType {
		Invalid,
		LegacyTle,
		Xml
	};

	// Allowed constructors.
	omm(QXmlStreamReader& r);
	omm(QString &, QString &);
	omm(QString&, QString&, QString&);

	// Destructor.
	virtual ~omm();

	//virtual omm& setLine0(const QString& s) { m_line0 = s; return *this; }
	//virtual omm& setLine1(const QString& s) { m_line1 = s; return *this; }
	//virtual omm& setLine2(const QString& s) { m_line2 = s; return *this; }

	virtual const QString& getLine0() { return m_line0; }
	virtual const QString& getLine1() { return m_line1; }
	virtual const QString& getLine2() { return m_line2; }

	bool hasValidLegacyTleData();

	bool setFromXML(QXmlStreamReader& r);

	virtual SourceType getSourceType() { return m_source_type; }

	virtual QDateTime getEpoch() { return m_epoch; }
	virtual const QString& getEpochStr() { return m_epoch_str; }
	virtual double getMeanMotion() { return m_mean_motion; }
	virtual double getEccentricity() { return m_eccentricity; }
	virtual double getInclination() { return m_inclination; }
	virtual double getAscendingNode() { return m_ascending_node; }
	virtual double getArgumentOfPerigee() { return m_argument_perigee; }
	virtual double getMeanAnomoly() { return m_mean_anomoly; }

	virtual QChar getClassification() { return m_classification; }
	virtual int getNoradcatId() { return m_norad_cat_id; }
	virtual int getElementSetNo() { return m_element_set_no; }
	virtual int getRevAtEpoch() { return m_rev_at_epoch; }
	virtual double getBstar() { return m_bstar; }
	virtual double getMeanMotionDot() { return m_mean_motion_dot; }
	virtual double getMeanMotionDDot() { return m_mean_motion_ddot; }

	virtual const QString& getObjectName() { return m_object_name; }
	virtual const QString& getObjectId() { return m_object_id; }

private:

	void processXmlElement(const QString& tag, const QString& val);

	SourceType m_source_type;
	
	// Legacy TLE data.
	QString m_line0{};
	QString m_line1{};
	QString m_line2{};

	// Mean elements.
	QDateTime m_epoch{};
	double m_mean_motion{};
	double m_eccentricity{};
	double m_inclination{};
	double m_ascending_node{};
	double m_argument_perigee{};
	double m_mean_anomoly{};

	// TLE parameters.
	QChar m_classification{};
	int m_norad_cat_id{};
	int m_element_set_no{};
	int m_rev_at_epoch{};
	double m_bstar{};
	double m_mean_motion_dot{};
	double m_mean_motion_ddot{};

	// Metadata
	QString m_object_name{};
	QString m_object_id{};

	// Misc data.
	QString m_epoch_str;
};

} // end namespace PluginSatellites

#endif
