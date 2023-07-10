/*
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

namespace PluginSatellites {

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
	processTleLegacy();
}

OMM::OMM(QString& l0, QString& l1, QString& l2)
{
	m_source_type = SourceType::LegacyTle;
	m_line0 = l0;
	m_line1 = l1;
	m_line2 = l2;
	processTleLegacy();
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
	if (tag == "EPOCH") {
		m_epoch_str = val;
		m_epoch = QDateTime::fromString(val, "yyyy-MM-ddThh:mm:ss.zzzzzz");
	}
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
	else if (tag == "ELEMENT_SET_NO")       m_element_set_no = val.toInt();
	else if (tag == "REV_AT_EPOCH")         m_rev_at_epoch = val.toInt();
	else if (tag == "BSTAR")                m_bstar = val.toDouble();
	else if (tag == "MEAN_MOTION_DOT")      m_mean_motion_dot = val.toDouble();
	else if (tag == "MEAN_MOTION_DDOT")     m_mean_motion_ddot = val.toDouble();
}

// Everything below here is for extracting the data from the two TLE lines.

//                              J   F   M   A   M   J   J   A   S   O   N   D
static int month_lens[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// TLE Line1 field positions and lengths.
static QPair<int, int> NORAD_CAT_ID(2,5);
static QPair<int, int> CLASSIFICATION_TYPE(7,1);
static QPair<int, int> OBJECT_ID(9, 8);
static QPair<int, int> EPOCH_YEAR(18, 2);
static QPair<int, int> EPOCH_DAY(20, 12);
static QPair<int, int> MEAN_MOTION_DOT(33, 10);
static QPair<int, int> MEAN_MOTION_DDOT(44, 8);
static QPair<int, int> BSTAR(53, 8);

// TLE Line2 field positions and lengths.
static QPair<int, int> INCLINATION(8, 8);
static QPair<int, int> RA_OF_ASC_NODE(17, 8);
static QPair<int, int> ECCENTRICITY(26, 7);
static QPair<int, int> ARG_OF_PERICENTER(34, 8);
static QPair<int, int> MEAN_ANOMALY(43, 8);
static QPair<int, int> MEAN_MOTION(52, 11);
static QPair<int, int> REV_AT_EPOCH(63, 5);

void OMM::processTleLegacy(void)
{
	if (m_line1.at(0) == '1') {
		m_norad_cat_id    = m_line1.mid(NORAD_CAT_ID.first, NORAD_CAT_ID.second).toInt();
		m_classification  = m_line1.at(CLASSIFICATION_TYPE.first);
		m_object_id       = m_line1.mid(OBJECT_ID.first, OBJECT_ID.second).trimmed();
		int epoch_year = m_line1.mid(EPOCH_YEAR.first, EPOCH_YEAR.second).toInt();
		if (epoch_year < 57) epoch_year += 2000;
		else epoch_year += 1900;
		QDate year = QDate(epoch_year, 1, 1);
		double epoch_day = m_line1.mid(EPOCH_DAY.first, EPOCH_DAY.second).toDouble();
		int day = std::floor(epoch_day);

		// 18-31 Epoch. Element Set Epoch (UTC) *Note: spaces are acceptable in columns 20 & 21
		m_mean_motion_dot = m_line1.mid(33, 10).toDouble();
		QString dec(".");
		dec.append(m_line1.mid(44, 5));
		m_mean_motion_ddot = dec.toDouble();
	}
}

} // end namespace PluginSatellites
