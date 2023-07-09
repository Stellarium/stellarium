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

#include <QDebug>

#include "omm.hpp"

namespace PluginSatellites {

omm::omm()
{
	m_source_type = SourceType::Invalid;
}

omm::~omm()
{}

omm::omm(QXmlStreamReader& r)
{
	m_source_type = SourceType::Invalid;
	setFromXML(r);
}

omm::omm(QString & l1, QString & l2)
{
	m_source_type = SourceType::LegacyTle;
	m_line1 = l1;
	m_line2 = l2;
}

omm::omm(QString& l0, QString& l1, QString& l2)
{
	m_source_type = SourceType::LegacyTle;
	m_line0 = l0;
	m_line1 = l1;
	m_line2 = l2;
}

bool omm::hasValidLegacyTleData()
{
	if(m_source_type == SourceType::LegacyTle) {
		if(m_line1.startsWith('1') && m_line2.startsWith('2')) {
			return true;
		}
	}
	return false;
}

bool omm::setFromXML(QXmlStreamReader & r)
{
	if (r.name().toString().toLower() == "omm") {
		bool collectChars = false;
		QString savedTag;
		QString savedVal;
		m_source_type = SourceType::Xml;
		r.readNext(); // Advance past starting <omm> tag.
		while (!r.hasError() && !r.atEnd()) {
			QString tag = r.name().toString();
			if (tag.toLower() == "omm") {
				// Detected </omm> closing tag.
				return true;
			}
			if (r.isStartElement()) {
				collectChars = true;
				savedTag = tag.toUpper();
				savedVal = "";
			} else if (collectChars) {
				if (r.isCharacters()) {
					savedVal += r.text();
				} else if (r.isEndElement()) {
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

void omm::processXmlElement(const QString & tag, const QString & val)
{
	if (tag == "OBJECT_NAME") {
		m_object_name = val;
	}
	else if (tag == "OBJECT_ID") {
		m_object_id = val;
	}
	else if (tag == "EPOCH") {
		m_epoch_str = val;
		m_epoch = QDateTime::fromString(val, "yyyy-MM-ddThh:mm:ss.zzzzzz");
	}
	else if (tag == "MEAN_MOTION") {
		m_mean_motion = val.toDouble();
	}
	else if (tag == "ECCENTRICITY") {
		m_eccentricity = val.toDouble();
	}
	else if (tag == "INCLINATION") {
		m_inclination = val.toDouble();
	}
	else if (tag == "RA_OF_ASC_NODE") {
		m_ascending_node = val.toDouble();
	}
	else if (tag == "ARG_OF_PERICENTER") {
		m_argument_perigee = val.toDouble();
	}
	else if (tag == "MEAN_ANOMALY") {
		m_mean_anomoly = val.toDouble();
	}
	else if (tag == "CLASSIFICATION_TYPE") {
		m_classification = val.at(0).toUpper();
	}
	else if (tag == "NORAD_CAT_ID") {
		m_norad_cat_id = val.toInt();
	}
	else if (tag == "ELEMENT_SET_NO") {
		m_element_set_no = val.toInt();
	}
	else if (tag == "REV_AT_EPOCH") {
		m_rev_at_epoch = val.toInt();
	}
	else if (tag == "BSTAR") {
		m_bstar = val.toDouble();
	}
	else if (tag == "MEAN_MOTION_DOT") {
		m_mean_motion_dot = val.toDouble();
	}
	else if (tag == "MEAN_MOTION_DDOT") {
		m_mean_motion_ddot = val.toDouble();
	}
}

} // end namespace PluginSatellites
