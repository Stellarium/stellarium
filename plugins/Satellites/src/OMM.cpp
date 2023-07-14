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

#include <cmath>
#include <limits>

#include <QPair>
#include <QDebug>

#include "OMM.hpp"

OMM::OMM()
{
	m_source_type = SourceType::Invalid;
}

OMM::~OMM()
{}

OMM::OMM(QJsonObject& o)
{
	m_source_type = SourceType::Json;
	setFromJsonObj(o);
}

OMM::OMM(QXmlStreamReader& r)
{
	m_source_type = SourceType::Invalid;
	setFromXML(r);
}

OMM::OMM(QString & l1, QString & l2)
{
	m_source_type = SourceType::LegacyTle;
	m_line1 = l1;
	m_line2 = l2;
	processTleLegacyLine1();
	processTleLegacyLine2();
}

OMM::OMM(QString& l0, QString& l1, QString& l2)
{
	m_source_type = SourceType::LegacyTle;
	m_line0 = l0;
	m_line1 = l1;
	m_line2 = l2;
	processTleLegacyLine0();
	processTleLegacyLine1();
	processTleLegacyLine2();
}

OMM::OMM(const OMM& other)
{
	*this = other;
}

OMM::OMM(const QVariantMap& m) 
{
	m_object_name = m.value("OBJECT_NAME").toString();
	m_norad_cat_id = m.value("NORAD_CAT_ID").toInt();
	m_classification = m.value("CLASSIFICATION_TYPE").toString().front().toUpper();
	m_object_id = m.value("OBJECT_ID").toString();
	m_mean_motion_dot = m.value("MEAN_MOTION_DOT").toDouble();
	m_mean_motion_ddot = m.value("MEAN_MOTION_DDOT").toDouble();
	m_bstar = m.value("BSTAR").toDouble();
	m_ephermeris_type = m.value("EPHEMERIS_TYPE").toDouble();
	m_element_number = m.value("ELEMENT_NUMBER").toDouble();
	m_inclination = m.value("INCLINATION").toDouble();
	m_ascending_node = m.value("RA_OF_ASC_NODE").toDouble();
	m_eccentricity = m.value("ECCENTRICITY").toDouble();
	m_argument_perigee = m.value("ARG_OF_PERICENTER").toDouble();
	m_mean_anomoly = m.value("MEAN_ANOMALY").toDouble();
	m_mean_motion = m.value("MEAN_MOTION").toDouble();
	m_rev_at_epoch = m.value("REV_AT_EPOCH").toDouble();
	m_epoch = OMMDateTime(m.value("EPOCH").toString());
}

OMM& OMM::operator=(const OMM &o)
{
	if(this == &o) {
		return *this;
	}
	
	m_source_type = o.m_source_type;
	m_line0 = o.m_line0;
	m_line1 = o.m_line1;
	m_line2 = o.m_line2;
	m_epoch = o.m_epoch;
	m_mean_motion = o.m_mean_motion;
	m_eccentricity = o.m_eccentricity;
	m_inclination = o.m_inclination;
	m_ascending_node = o.m_ascending_node;
	m_argument_perigee = o.m_argument_perigee;
	m_mean_anomoly = o.m_mean_anomoly;
	m_classification = o.m_classification;
	m_norad_cat_id = o.m_norad_cat_id;
	m_rev_at_epoch = o.m_rev_at_epoch;
	m_bstar = o.m_bstar;
	m_mean_motion_dot = o.m_mean_motion_dot;
	m_mean_motion_ddot = o.m_mean_motion_ddot;
	m_ephermeris_type = o.m_ephermeris_type;
	m_element_number = o.m_element_number;
	m_object_name = o.m_object_name;
	m_object_id = o.m_object_id;
	m_status = o.m_status;
	return *this;
}

void OMM::toJsonObj(QJsonObject& rval)
{
	rval.insert("OBJECT_NAME", QJsonValue(m_object_name));
	rval.insert("NORAD_CAT_ID", QJsonValue(m_norad_cat_id));
	rval.insert("CLASSIFICATION_TYPE", QJsonValue(m_classification));
	rval.insert("OBJECT_ID", QJsonValue(m_object_id));
	rval.insert("EPOCH", QJsonValue(m_epoch.toISO8601String()));
	rval.insert("MEAN_MOTION_DOT", QJsonValue(m_mean_motion_dot));
	rval.insert("MEAN_MOTION_DDOT", QJsonValue(m_mean_motion_ddot));
	rval.insert("BSTAR", QJsonValue(m_bstar));
	rval.insert("EPHEMERIS_TYPE", QJsonValue(m_ephermeris_type));
	rval.insert("ELEMENT_NUMBER", QJsonValue(m_element_number));
	rval.insert("INCLINATION", QJsonValue(m_inclination));
	rval.insert("RA_OF_ASC_NODE", QJsonValue(m_ascending_node));
	rval.insert("ECCENTRICITY", QJsonValue(m_eccentricity));
	rval.insert("ARG_OF_PERICENTER", QJsonValue(m_argument_perigee));
	rval.insert("MEAN_ANOMALY", QJsonValue(m_mean_anomoly));
	rval.insert("MEAN_MOTION", QJsonValue(m_mean_motion));
	rval.insert("REV_AT_EPOCH", QJsonValue(m_rev_at_epoch));
}

void OMM::toVariantMap(QVariantMap & outmap)
{
	outmap.insert("OBJECT_NAME", QVariant(m_object_name));
	outmap.insert("NORAD_CAT_ID", QVariant(m_norad_cat_id));
	outmap.insert("CLASSIFICATION_TYPE", QVariant(m_classification));
	outmap.insert("OBJECT_ID", QVariant(m_object_id));
	outmap.insert("EPOCH", QVariant(m_epoch.toISO8601String()));
	outmap.insert("MEAN_MOTION_DOT", QVariant(m_mean_motion_dot));
	outmap.insert("MEAN_MOTION_DDOT", QVariant(m_mean_motion_ddot));
	outmap.insert("BSTAR", QVariant(m_bstar));
	outmap.insert("EPHEMERIS_TYPE", QVariant(m_ephermeris_type));
	outmap.insert("ELEMENT_NUMBER", QVariant(m_element_number));
	outmap.insert("INCLINATION", QVariant(m_inclination));
	outmap.insert("RA_OF_ASC_NODE", QVariant(m_ascending_node));
	outmap.insert("ECCENTRICITY", QVariant(m_eccentricity));
	outmap.insert("ARG_OF_PERICENTER", QVariant(m_argument_perigee));
	outmap.insert("MEAN_ANOMALY", QVariant(m_mean_anomoly));
	outmap.insert("MEAN_MOTION", QVariant(m_mean_motion));
	outmap.insert("REV_AT_EPOCH", QVariant(m_rev_at_epoch));
}

bool OMM::hasValidLegacyTleData()
{
	if(m_source_type == SourceType::LegacyTle) {
		if(m_line1.startsWith('1') && m_line2.startsWith('2')) {
			return true;
		}
	}
	return false;
}

double OMM::getEpochJD()
{
	return m_epoch.getJulian();
}

bool OMM::setFromJsonObj(const QJsonObject & obj)
{
	foreach (const QString& key, obj.keys()) {
		QJsonValue val = obj.value(key);
		if(val.isUndefined() || val.isNull()) {
			qDebug() << "Undefined value for key: " << key;
		}
		else {
			processTagElement(key, val);
		}
	}
	return true;
}

bool OMM::setFromXML(QXmlStreamReader & r)
{
	if (r.name().toString().toLower() == "omm") {
		bool collectChars = false;
		QString savedTag;
		QString savedVal;
		m_source_type = SourceType::Xml;
		r.readNext(); // Advance past starting <OMM> tag.
		while (!r.hasError() && !r.atEnd()) {
			QString tag = r.name().toString();
			if (tag.toLower() == "omm") {
				// Detected </OMM> closing tag.
				return true;
			}
			if (r.isStartElement()) {
				collectChars = true;
				savedTag = tag.toUpper();
				savedVal = "";
			} 
			else if (collectChars) {
				if (r.isCharacters()) {
					savedVal += r.text();
				} 
				else if (r.isEndElement()) {
					if(!savedTag.isEmpty() && savedTag.size() > 0) {
						QJsonValue val(savedVal);
						processTagElement(savedTag, val);
					}
					collectChars = false;
					savedVal = "";
				}
			}
			r.readNext();
		}
	}
	if (r.hasError()) {
		QString e = r.errorString();
		qDebug() << e;
	}
	m_source_type = SourceType::Invalid;
	return false;
}



// Everything below here is for extracting the data from the legacy two line TLE format.

void OMM::processTleLegacyLine0(void)
{
	if (!m_line0.isEmpty()) {
		m_object_name = m_line0.trimmed();
	}
}

// TLE Line1 field positions and lengths.
static QPair<int, int> NORAD_CAT_ID(2,5);
static QPair<int, int> CLASSIFICATION_TYPE(7,1);
static QPair<int, int> OBJECT_ID(9, 8);
static QPair<int, int> EPOCH(18, 14);
static QPair<int, int> EPOCH_YEAR(18, 2);
static QPair<int, int> EPOCH_DAY(20, 12);
static QPair<int, int> MEAN_MOTION_DOT(33, 10);
static QPair<int, int> MEAN_MOTION_DDOT(44, 8);
static QPair<int, int> BSTAR(53, 8);
static QPair<int, int> EPHEMERIS_TYPE(62, 1);
static QPair<int, int> ELEMENT_NUMBER(64, 4);

void OMM::processTleLegacyLine1(void)
{
	if (!m_line1.isEmpty() && m_line1.at(0) == '1') {
		auto epoch_str    = m_line1.mid(EPOCH.first, EPOCH.second).trimmed();
		m_epoch           = OMMDateTime(epoch_str);
 		m_norad_cat_id    = m_line1.mid(NORAD_CAT_ID.first, NORAD_CAT_ID.second).toInt();
		m_classification  = m_line1.at(CLASSIFICATION_TYPE.first);
		m_object_id       = m_line1.mid(OBJECT_ID.first, OBJECT_ID.second).trimmed();
		m_mean_motion_dot = m_line1.mid(33, 10).toDouble();
		QString ddot(".");
		ddot.append(m_line1.mid(44, 5));
		ddot.replace(QChar('-'), QString("e-"));
		m_mean_motion_ddot = ddot.toDouble();
		QString bstar(".");
		bstar.append(m_line1.mid(BSTAR.first, BSTAR.second).trimmed());
		bstar.replace(QChar('-'), QString("e-"));
		m_bstar = bstar.toDouble();
		m_ephermeris_type = m_line1.mid(EPHEMERIS_TYPE.first, EPHEMERIS_TYPE.second).trimmed().toInt();
		m_element_number  = m_line1.mid(ELEMENT_NUMBER.first, ELEMENT_NUMBER.second).trimmed().toInt();
	}
}

// TLE Line2 field positions and lengths.
static QPair<int, int> INCLINATION(8, 8);
static QPair<int, int> RA_OF_ASC_NODE(17, 8);
static QPair<int, int> ECCENTRICITY(26, 7);
static QPair<int, int> ARG_OF_PERICENTER(34, 8);
static QPair<int, int> MEAN_ANOMALY(43, 8);
static QPair<int, int> MEAN_MOTION(52, 11);
static QPair<int, int> REV_AT_EPOCH(63, 5);

void OMM::processTleLegacyLine2(void)
{
	if(!m_line2.isEmpty() && m_line2.at(0) == '2') {
		m_inclination = m_line2.mid(INCLINATION.first, INCLINATION.second).trimmed().toDouble();
		m_ascending_node = m_line2.mid(RA_OF_ASC_NODE.first, RA_OF_ASC_NODE.second).trimmed().toDouble();
		m_argument_perigee = m_line2.mid(ARG_OF_PERICENTER.first, ARG_OF_PERICENTER.second).trimmed().toDouble();
		QString ecc_s(".");
		ecc_s.append(m_line2.mid(ECCENTRICITY.first, ECCENTRICITY.second));
		m_eccentricity = ecc_s.trimmed().toDouble();
		m_mean_anomoly = m_line2.mid(MEAN_ANOMALY.first, MEAN_ANOMALY.second).trimmed().toDouble();
		m_mean_motion = m_line2.mid(MEAN_MOTION.first, MEAN_MOTION.second).trimmed().toDouble();
		m_rev_at_epoch = m_line2.mid(REV_AT_EPOCH.first, REV_AT_EPOCH.second).trimmed().toInt();
	}
}

// OMM JSON come with "real" type information but in OMM XML everything is a string.
// Each setter must check for unexpected string typing from XML and convert accordingly.
// It's a shame that QJsonValue that is a string the toInt() and toDouble() do not 
// work as you might think. Hence the toString().toInt/Double() shinanigans.

bool OMM::setEpoch(const QJsonValue& val, const QString& tag)
{
	if(tag.isEmpty() || tag == "EPOCH") {
		m_epoch = OMMDateTime(val.toString(), OMMDateTime::STR_ISO8601);
		return true;
	}
	return false;
}

bool OMM::setObjectName(const QJsonValue& val, const QString& tag)
{
	if(tag.isEmpty() || tag == "OBJECT_NAME") {
		m_object_name = val.toString();
		return true;
	}
	return false;
}

bool OMM::setObjectId(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "OBJECT_ID") {
		m_object_id = val.toString();
		return true;
	}
	return false;
}

bool OMM::setMeanMotion(const QJsonValue& val, const QString& tag)
{
	if(tag.isEmpty() || tag == "MEAN_MOTION") {
		m_mean_motion = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setEccentricity(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "ECCENTRICITY") {
		m_eccentricity = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setInclination(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "INCLINATION") {
		m_inclination = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setAscendingNode(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "RA_OF_ASC_NODE") {
		m_ascending_node = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setArgumentOfPerigee(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "ARG_OF_PERICENTER") {
		m_argument_perigee = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setMeanAnomoly(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "MEAN_ANOMALY") {
		m_mean_anomoly = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setClassification(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "CLASSIFICATION_TYPE") {
		m_classification = val.toString().at(0).toUpper();
		return true;
	}
	return false;
}

bool OMM::setNoradcatId(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "NORAD_CAT_ID") {
		m_norad_cat_id = val.isString() ?
			val.toString().toInt() : val.toInt();
		return true;
	}
	return false;
}

bool OMM::setRevAtEpoch(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "REV_AT_EPOCH") {
		m_rev_at_epoch = val.isString() ?
			val.toString().toInt() : val.toInt();
		return true;
	}
	return false;
}

bool OMM::setElementNumber(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "ELEMENT_SET_NO") {
		m_element_number = val.isString() ?
			val.toString().toInt() : val.toInt();
		return true;
	}
	return false;
}

bool OMM::setBstar(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "BSTAR") {
		m_bstar = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setMeanMotionDot(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "MEAN_MOTION_DOT") {
		m_mean_motion_dot = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

bool OMM::setMeanMotionDDot(const QJsonValue & val, const QString & tag)
{
	if (tag.isEmpty() || tag == "MEAN_MOTION_DDOT") {
		m_mean_motion_ddot = val.isString() ?
			val.toString().toDouble() : val.toDouble();
		return true;
	}
	return false;
}

/*
* Create a table of setter methods to loop over
* to allow setting properties via tag name.
*/

// Function/method pointer type.
typedef bool (OMM::*setter)(const QJsonValue&, const QString&);

// Table of function/method pointers
static setter setFuncs[] 
{ 
	&OMM::setEpoch,
	&OMM::setObjectName,
	&OMM::setObjectId,
	&OMM::setMeanMotion,
	&OMM::setEccentricity, 
	&OMM::setInclination, 
	&OMM::setAscendingNode,
	&OMM::setArgumentOfPerigee,
	&OMM::setMeanAnomoly,
	&OMM::setClassification, 
	&OMM::setRevAtEpoch,
	&OMM::setElementNumber,
	&OMM::setBstar,
	&OMM::setMeanMotionDot,
	&OMM::setMeanMotionDDot,
	&OMM::setNoradcatId, // Keep last in table.
	nullptr
};

// Number of entries in table.
#define TABLEN ((sizeof(setFuncs)/sizeof(setter))-1)

// Looped setter from tag.
void OMM::processTagElement(const QString& tag, const QJsonValue& val)
{
	if(!tag.isEmpty() && tag.size() > 0) {
		int idx = 0;
		setter set;
		while (idx < TABLEN) {
			set = setFuncs[idx++];
			if (set == nullptr) return;
			if ((this->*set)(val, tag) == true) return;
		}
	}
}
