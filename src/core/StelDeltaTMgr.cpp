/*
 * Stellarium
 * Copyright (C) 2014  Bogdan Marinov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "StelDeltaTMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "DeltaTAlgorithm.hpp"

#include <QDebug>

using namespace DeltaT;

StelDeltaTMgr::StelDeltaTMgr() :
    currentAlgorithm(0),
    defaultAlgorithm(0),
    zeroAlgorithm(0),
    customAlgorithm(0)
{
	zeroAlgorithm = new WithoutCorrection();
	currentAlgorithm = zeroAlgorithm;
	customAlgorithm = new Custom();
	DeltaTAlgorithm* newAlgorithm = 0;

	algorithms.insert(zeroAlgorithm->getId(), zeroAlgorithm);
	algorithms.insert(customAlgorithm->getId(), customAlgorithm);
	// TODO

	// TODO: Add the actual one
	defaultAlgorithm = zeroAlgorithm;
}

StelDeltaTMgr::~StelDeltaTMgr()
{
	currentAlgorithm = 0;
	defaultAlgorithm = 0;
	zeroAlgorithm = 0;
	customAlgorithm = 0;
	qDeleteAll(algorithms);
	algorithms.clear();
}

void
StelDeltaTMgr::setCurrentAlgorithm(const QString& id)
{
	// TODO: Or should it revert to the default instead?
	currentAlgorithm = algorithms.value(id, zeroAlgorithm);
	if (currentAlgorithm->getId() != id)
		qWarning() << "WARNING: Unable to find DeltaT algorithm" << id << "; "
		           << "using" << currentAlgorithm->getId() << "instead.";
}

QString
StelDeltaTMgr::getCurrentAlgorithmId() const
{
	return currentAlgorithm->getId();
}

QList<QString>
StelDeltaTMgr::getAvailableAlgorithmIds() const
{
	return algorithms.keys();
}

void StelDeltaTMgr::populateAvailableAlgorithmsModel(QAbstractItemModel* baseModel)
{
	QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(baseModel);
	if(model == NULL)
		return;
	model->clear();

	QMapIterator<QString,DeltaTAlgorithm*> a(algorithms);
	while (a.hasNext())
	{
		a.next();

		QStandardItem* item = new QStandardItem(a.value()->getName());
		//item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren);
		//QComboBox uses UserRole by default
		item->setData(a.key(), Qt::UserRole);

		QString description = a.value()->getDescription();
		QString range = a.value()->getRangeDescription();
		if (!range.isEmpty())
		{
			description.append("<br/>");
			description.append(range);
		}
		if (a.value() == defaultAlgorithm)
		{
			description.append("<br/></br/>\n");
			description.append("<strong>");
			// TRANSLATORS: Indicates the default DeltaT algorithm in the GUI.
			description.append(q_("Used by default."));
			description.append("</strong>");

//			QFont font = item->font();
//			font.setBold(true);
//			item->setFont(font);
		}
		item->setData(description, Qt::UserRole + 1);

		bool isCustom = (a.value() == customAlgorithm);
		item->setData(isCustom, Qt::UserRole + 2);

		model->appendRow(item);
	}
}

double
StelDeltaTMgr::calculateDeltaT(const double& jdUtc, QString* outputString)
{
	int year, month, day;
	StelUtils::getDateFromJulianDay(jdUtc, &year, &month, &day);
	return currentAlgorithm->calculateDeltaT(jdUtc,
	                                         year, month, day,
	                                         outputString);
}

void StelDeltaTMgr::getCustomAlgorithmParams(float* year,
                                             float* ndot,
                                             float* a,
                                             float* b,
                                             float* c)
{
	Custom* algorithm = dynamic_cast<Custom*>(customAlgorithm);
	if (algorithm)
	{
		;// TODO
	}
}

void
StelDeltaTMgr::setCustomAlgorithmParams(const float& year,
                                        const float& ndot,
                                        const float& a,
                                        const float& b,
                                        const float& c)
{
	Custom* algorithm = dynamic_cast<Custom*>(customAlgorithm);
	if (algorithm)
		algorithm->setParameters(year, ndot, a, b, c);
}
