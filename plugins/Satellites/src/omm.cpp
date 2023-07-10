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

#include <QPair>
#include <QDebug>

#include "OMM.hpp"

OMM::OMM()
{
	m_source_type = SourceType::Invalid;
}

OMM::~OMM()
{}

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

bool OMM::hasValidLegacyTleData()
{
	if(m_source_type == SourceType::LegacyTle) {
		if(m_line1.startsWith('1') && m_line2.startsWith('2')) {
			return true;
		}
	}
	return false;
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
					processXmlElement(savedTag, savedVal);
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

void OMM::processXmlElement(const QString & tag, const QString & val)
{
	if (tag == "EPOCH")                     m_sp_epoch = OMMDateTime::ShPtr(new OMMDateTime(val, OMMDateTime::STR_ISO8601));
	else if (tag == "OBJECT_NAME")          m_object_name = val;
	else if (tag == "OBJECT_ID")            m_object_id = val;
	else if (tag == "MEAN_MOTION")          m_mean_motion = val.toDouble();
	else if (tag == "ECCENTRICITY")         m_eccentricity = val.toDouble();
	else if (tag == "INCLINATION")          m_inclination = val.toDouble();
	else if (tag == "RA_OF_ASC_NODE")       m_ascending_node = val.toDouble();
	else if (tag == "ARG_OF_PERICENTER")    m_argument_perigee = val.toDouble();
	else if (tag == "MEAN_ANOMALY")         m_mean_anomoly = val.toDouble();
	else if (tag == "CLASSIFICATION_TYPE")  m_classification = val.at(0).toUpper();
	else if (tag == "NORAD_CAT_ID")         m_norad_cat_id = val.toInt();
	else if (tag == "ELEMENT_SET_NO")       m_element_number = val.toInt();
	else if (tag == "REV_AT_EPOCH")         m_rev_at_epoch = val.toInt();
	else if (tag == "BSTAR")                m_bstar = val.toDouble();
	else if (tag == "MEAN_MOTION_DOT")      m_mean_motion_dot = val.toDouble();
	else if (tag == "MEAN_MOTION_DDOT")     m_mean_motion_ddot = val.toDouble();
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
		m_sp_epoch        = OMMDateTime::ShPtr(new OMMDateTime(epoch_str));
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
