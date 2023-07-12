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

#include "OMMDownload.hpp"

OMMDownload::OMMDownload()
{}

OMMDownload::~OMMDownload()
{
	if(m_ptim != nullptr) {
		delete m_ptim;
	}
}

void OMMDownload::execute(void)
{
	if (m_sim == sim_t::SIM_REMOTE) {
		executeSim();
	}
	else {
		executeSim();
		// Simulators.
/*
		switch(m_sim) {
			default:
			case sim_t::SIM_LOCAL_INLINE:
				executeSim();
				break;
		}
*/
	}
}

void OMMDownload::executeSim(void)
{
	m_current_itor = m_uriMap.begin();
	m_current_key = m_current_itor.key();
	m_current_val = m_current_itor.value();
	m_ptim = new QTimer;
	m_ptim->setInterval(100);
	connect(m_ptim, &QTimer::timeout, this, &OMMDownload::slotSimulateEnd);
	m_ptim->start();
}

void OMMDownload::slotSimulateEnd(void)
{
	emit fileDownloadComplete(m_current_key);
	m_current_itor++;
	if(m_current_itor != m_uriMap.end()) {
		m_current_key = m_current_itor.key();
		m_current_val = m_current_itor.value();
	}
	else {
		m_ptim->stop();
		emit executeComplete();
	}
}
